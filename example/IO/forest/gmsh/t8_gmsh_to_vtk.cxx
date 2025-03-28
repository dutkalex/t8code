/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element types in parallel.

  Copyright (C) 2024 the developers

  t8code is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  t8code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with t8code; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/* This file provides a utility routine for quickly reading in Gmsh files and
 * writing them out to a VTU file for visualization.
 */

#include <t8.h>                                 /* General t8code header, always include this. */
#include <sc_options.h>                         /* CLI parser. */
#include <t8_cmesh.h>                           /* Cmesh definition and basic interface. */
#include <t8_forest/t8_forest_general.h>        /* Create forest. */
#include <t8_forest/t8_forest_io.h>             /* Save forest. */
#include <t8_schemes/t8_default/t8_default.hxx> /* Default refinement scheme. */
#include <t8_cmesh_readmshfile.h>               /* Msh file reader. */

int
main (int argc, char **argv)
{
  sc_options_t *opt;   // The options we want to parse.
  char help[BUFSIZ];   // Help message.
  int helpme;          // Print help message.
  int parsed;          // Return value of sc_options_parse.
  int sreturn;         // Return value of sc functions.
  int mpiret;          // Return value of MPI functions.
  sc_MPI_Comm comm;    // The MPI communicator.
  t8_cmesh_t cmesh;    // The cmesh we read in from the msh file.
  t8_forest_t forest;  // The forest we want to refine.

  const char *fileprefix = NULL;  // The prefix of the msh (and brep) file.
  int level;                      // Uniform refinement level of the forest.
  int dim;                        // The dimension of the mesh.
  int use_cad;                    // Use CAD-based curvilinear geometry.

  // Help message.
  sreturn = snprintf (help, BUFSIZ, "Read in a `.msh` file generated by Gmsh and write it into a VTU file.\n");

  if (sreturn >= BUFSIZ) {
    // The help message was truncated.
    // Note: gcc >= 7.1 prints a warning if we
    // do not check the return value of snprintf.
    t8_debugf ("Warning: Truncated help message to '%s'\n", help);
  }

  // Initialize MPI. This has to happen before we initialize sc or t8code.
  mpiret = sc_MPI_Init (&argc, &argv);
  // Error check the MPI return value.
  SC_CHECK_MPI (mpiret);

  // Initialize the sc library, has to happen before we initialize t8code.
  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
  // Initialize t8code with log level SC_LP_PRODUCTION. See sc.h for more info on the log levels.
  t8_init (SC_LP_ESSENTIAL);

  // We will use MPI_COMM_WORLD as a communicator.
  comm = sc_MPI_COMM_WORLD;

  // Initialize command line argument parser.
  opt = sc_options_new (argv[0]);
  sc_options_add_switch (opt, 'h', "help", &helpme, "Display a short help message.");
  sc_options_add_string (opt, 'f', "fileprefix", &fileprefix, NULL, "Fileprefix of the msh and brep files.");
  sc_options_add_int (opt, 'l', "level", &level, 2, "The uniform refinement level. Default: 2");
  sc_options_add_int (opt, 'd', "dimension", &dim, 3, "The dimension of the mesh. Default: 3");
  sc_options_add_switch (opt, 'c', "use_cad", &use_cad,
                         "Enable CAD-based curvilinear geometry. Needs a `.brep` file with the same file prefix.");
  parsed = sc_options_parse (t8_get_package_id (), SC_LP_ERROR, opt, argc, argv);

  if (helpme) {
    t8_global_errorf ("%s\n", help);
    sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
  }
  else if (parsed == 0 || fileprefix == NULL) {
    t8_global_productionf ("\n\tERROR: Wrong usage.\n\n");
    sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
  }
  else {
    // Read in the msh file.
    cmesh = t8_cmesh_from_msh_file (fileprefix, 0, sc_MPI_COMM_WORLD, dim, 0, use_cad);

    // Construct a forest from the cmesh.
    forest = t8_forest_new_uniform (cmesh, t8_scheme_new_default (), level, 0, comm);
    T8_ASSERT (t8_forest_is_committed (forest));

    {
      // Write user defined data to vtu file.
      const int write_treeid = 1;
      const int write_mpirank = 1;
      const int write_level = 1;
      const int write_element_id = 1;
      const int write_ghosts = 0;
#if T8_ENABLE_VTK
      const int write_curved = 1;
#else
      const int write_curved = 0;
#endif
      const int do_not_use_api = 0;
      const int num_data = 0;
      t8_forest_write_vtk_ext (forest, fileprefix, write_treeid, write_mpirank, write_level, write_element_id,
                               write_ghosts, write_curved, do_not_use_api, num_data, NULL);

      t8_global_productionf ("Wrote %s.\n", fileprefix);
    }

    t8_forest_unref (&forest);
  }

  sc_options_destroy (opt);
  sc_finalize ();
  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
  return 0;
}
