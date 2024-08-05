#include <sc_options.h>
#include <t8.h>
#include <t8_forest/t8_forest.h>
#include <t8_schemes/t8_transition/t8_transition_cxx.hxx>
#include <t8_cmesh/t8_cmesh_examples.h>
#include <t8_cmesh_readmshfile.h>
#include <vector>
#include <algorithm>

int
tabeamug_adapt (t8_forest_t forest, t8_forest_t forest_from, t8_locidx_t which_tree, t8_locidx_t lelement_id,
                t8_eclass_scheme_c *ts, const int is_family, const int num_elements, t8_element_t *elements[])
{
  const int maxlevel = *(const int *) t8_forest_get_user_data (forest);
  const t8_gloidx_t tree_boundariesA[] = { 0, 1 };  //{ 10, 40 };
  const t8_gloidx_t tree_boundariesB[] = { 90, 120 };

  const int element_level = ts->t8_element_level (elements[0]);
  if (element_level >= maxlevel) {
    return 0;  // Do not refine beyond maximum level.
  }
  const t8_gloidx_t global_tree_id = t8_forest_global_tree_id (forest_from, which_tree);
  const bool is_in_treeset_A = tree_boundariesA[0] <= global_tree_id && global_tree_id < tree_boundariesA[1];
  const bool is_in_treeset_B = tree_boundariesB[0] <= global_tree_id && global_tree_id < tree_boundariesB[1];
  if (is_in_treeset_A || is_in_treeset_B) {
    // This element is in a tree that should be refined.
    return 1;
  }
  return 0;
}

void
tabeamug_build_forest (const char *filename, int level, int maxlevel)
{
  sc_MPI_Comm comm = sc_MPI_COMM_WORLD;

  /* Build the cmesh from meshfile using CAD. (requires files "filename.msh" and "filename.brep"). */
  const int partition = 0;
  const int dimension = 2;
  const int main_rank = 0;
  const int use_cad = 0;

  t8_cmesh_t cmesh = t8_cmesh_from_msh_file (filename, partition, comm, dimension, main_rank, use_cad);

  SC_CHECK_ABORTF (cmesh != NULL, "Could not build cmesh from files %s.msh and %s.brep.\n", filename, filename);

  /* Build uniform forest */
  t8_scheme_cxx_t *scheme = t8_scheme_new_transition_quad_cxx ();  // default adapt scheme.

  const int do_face_ghost = 0;  // No ghost needed.
  t8_forest_t forest_uniform = t8_forest_new_uniform (cmesh, scheme, level, do_face_ghost, comm);

  t8_forest_t forest_temp = forest_uniform;
  t8_forest_t forest_adapt = forest_uniform;
  /* Build adapted forest */
  for (int ilevel = level + 1; ilevel <= maxlevel; ++ilevel) {
    const int recursive = 0;  // We want to define multiple levels at once.
    /* Adapt the forest. We use the maximum refinement levet as user data. */
    forest_adapt = t8_forest_new_adapt (forest_temp, tabeamug_adapt, recursive, do_face_ghost, &maxlevel);
    forest_temp = forest_adapt;
  }

  t8_eclass_scheme_c *quad_scheme = t8_forest_get_eclass_scheme (forest_adapt, T8_ECLASS_QUAD);
  const t8_element_t *elem = t8_forest_get_element (forest_adapt, 0, NULL);
  double ref_coords[3] = { 0, 0, -1 };
  double out_coords[2];
  quad_scheme->t8_element_reference_coords (elem, ref_coords, 1, out_coords);

  // vtk output of adapted forest
  char vtkname[BUFSIZ];
  snprintf (vtkname, BUFSIZ, "tabeamug_adapt_%i_%i", level, maxlevel);
  //t8_forest_write_vtk_ext (forest_adapt, vtkname, 1, 1, 1, 1, 1, 0, 1, 0, NULL);
  t8_forest_write_vtk (forest_adapt, vtkname);

  // builde balanced forest
  t8_forest_t forest_balance;
  t8_forest_init (&forest_balance);
  const int no_repartition = 0;
  t8_forest_set_balance (forest_balance, forest_adapt, no_repartition);
  t8_forest_commit (forest_balance);

  // vtk output of balanced forest
  snprintf (vtkname, BUFSIZ, "tabeamug_balance_%i_%i", level, maxlevel);
  //t8_forest_write_vtk_ext (forest_adapt, vtkname, 1, 1, 1, 1, 1, 0, 1, 0, NULL);
  //t8_forest_write_vtk (forest_balance, vtkname);

  // build transitioned (no hanging nodes) forest
  t8_forest_t forest_transition;
  t8_forest_init (&forest_transition);
  int do_extra_balance = 0;  // not necessary since the input forest is balanced.
  t8_forest_set_transition (forest_transition, forest_balance, do_extra_balance);
  t8_forest_commit (forest_transition);

  // vtk output of transitioned forest
  snprintf (vtkname, BUFSIZ, "tabeamug_transition_%i_%i", level, maxlevel);
  //t8_forest_write_vtk_ext (forest_transition, vtkname, 1, 1, 1, 1, 1, 0, 1, 0, NULL);
  t8_forest_write_vtk (forest_transition, vtkname);

  t8_forest_unref (&forest_transition);
}

int
main (int argc, char *argv[])
{
  int mpiret;
  int level = 0;
  int maxlevel = 0;
  int helpme = 0;
  int parsed = 0;
  sc_options_t *opt;
  const char *filename;
  const char *help = "This program was written to create a specific adaptive mesh for a mug design as parting gift for "
                     "our colleague Tabea.\n"
                     "The program reads a .msh file and builds an adaptive mesh from it.\n"
                     "The mesh is refined on specifically defined trees.\n";

#if T8_ENABLE_DEBUG
  printf ("=============DEBUG ON==============\n");
#else
  printf ("=============DEBUG OFF=============\n");
#endif

  /* Initialize MPI. This has to happen before we initialize sc or t8code. */
  mpiret = sc_MPI_Init (&argc, &argv);
  /* Error check the MPI return value. */
  SC_CHECK_MPI (mpiret);

  /* Initialize the sc library, has to happen before we initialize t8code. */
  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_PRODUCTION);
  /* Initialize t8code with log level SC_LP_PRODUCTION. See sc.h for more info on the log levels. */
  t8_init (SC_LP_TRACE);

  /* initialize command line argument parser */
  opt = sc_options_new (argv[0]);
  sc_options_add_switch (opt, 'h', "help", &helpme, "Display a short help message.");
  sc_options_add_string (opt, 'f', "file", &filename, "tennis", "msh and cad file prefix. Default \'tennis'\n");
  sc_options_add_int (opt, 'l', "level", &level, 0, "The initial refinement level of the mesh. Default 0.");
  sc_options_add_int (opt, 'm', "maxlevel", &maxlevel, 5,
                      "The maximum allowed refinement level of the mesh. Default 5.");

  parsed = sc_options_parse (-1, SC_LP_ERROR, opt, argc, argv);
  if (helpme) {
    /* display help message and usage */
    t8_global_productionf ("%s\n", help);
    sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
  }
  else if (parsed >= 0 && 0 <= level && strcmp (filename, "") && level <= maxlevel) {
    tabeamug_build_forest (filename, level, maxlevel);
  }
  else {
    /* wrong usage */
    t8_global_productionf ("\n\t ERROR: Wrong usage.\n\n");
    sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
  }

  sc_options_destroy (opt);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
  return 0;
}
