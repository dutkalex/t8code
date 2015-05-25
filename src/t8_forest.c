/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2015 the developers

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

#include <sc_refcount.h>
#include <t8_forest.h>

typedef enum t8_forest_from
{
  T8_FOREST_FROM_FIRST,
  T8_FOREST_FROM_COPY = T8_FOREST_FROM_FIRST,
  T8_FOREST_FROM_ADAPT,
  T8_FOREST_FROM_PARTITION,
  T8_FOREST_FROM_LAST
}
t8_forest_from_t;

/** This structure is private to the implementation. */
typedef struct t8_forest
{
  sc_refcount_t       rc;               /**< Reference counter. */

  int                 set_level;        /**< Level to use in new construction. */
  int                 set_for_coarsening;       /**< Change partition to allow
                                                     for one round of coarsening */

  sc_MPI_Comm         mpicomm;          /**< MPI communicator to use. */
  t8_cmesh_t          cmesh;            /**< Coarse mesh to use. */
  t8_scheme_t        *scheme;           /**< Scheme for element types. */
  int                 do_dup;           /**< Communicator shall be duped. */
  int                 dimension;        /**< Dimension inferred from \b cmesh. */

  t8_forest_t         set_from;         /**< Temporarily store source forest. */
  t8_forest_from_t    from_method;      /**< Method to derive from \b set_from. */

  int                 constructed;      /**< \ref t8_forest_construct called? */
  int                 mpisize;          /**< Number of MPI processes. */
  int                 mpirank;          /**< Numbor of this MPI process. */
}
t8_forest_struct_t;

void
t8_forest_new (t8_forest_t * pforest)
{
  t8_forest_t         forest;

  T8_ASSERT (pforest != NULL);

  forest = *pforest = T8_ALLOC_ZERO (t8_forest_struct_t, 1);
  sc_refcount_init (&forest->rc);

  /* sensible (hard error) defaults */
  forest->mpicomm = sc_MPI_COMM_NULL;
  forest->dimension = -1;
  forest->from_method = T8_FOREST_FROM_LAST;

  forest->mpisize = -1;
  forest->mpirank = -1;
}

void
t8_forest_set_mpicomm (t8_forest_t forest, sc_MPI_Comm mpicomm, int do_dup)
{
  T8_ASSERT (forest != NULL);
  T8_ASSERT (!forest->constructed);
  T8_ASSERT (forest->mpicomm == sc_MPI_COMM_NULL);
  T8_ASSERT (forest->set_from == NULL);
  /* TODO: check positive reference count in all functions */

  T8_ASSERT (mpicomm != sc_MPI_COMM_NULL);

  forest->mpicomm = mpicomm;
  forest->do_dup = do_dup;
}

void
t8_forest_set_cmesh (t8_forest_t forest, t8_cmesh_t cmesh)
{
  T8_ASSERT (forest != NULL);
  T8_ASSERT (!forest->constructed);
  T8_ASSERT (forest->cmesh == NULL);
  T8_ASSERT (forest->set_from == NULL);

  T8_ASSERT (cmesh != NULL);

  forest->cmesh = cmesh;
}

void
t8_forest_set_scheme (t8_forest_t forest, t8_scheme_t * scheme)
{
  T8_ASSERT (forest != NULL);
  T8_ASSERT (!forest->constructed);
  T8_ASSERT (forest->scheme == NULL);
  T8_ASSERT (forest->set_from == NULL);

  T8_ASSERT (scheme != NULL);

  forest->scheme = scheme;
}

void
t8_forest_set_level (t8_forest_t forest, int level)
{
  T8_ASSERT (forest != NULL);
  T8_ASSERT (!forest->constructed);

  T8_ASSERT (0 <= level);

  forest->set_level = level;
}

void
t8_forest_set_copy (t8_forest_t forest, const t8_forest_t set_from)
{
  T8_ASSERT (forest != NULL);
  T8_ASSERT (!forest->constructed);
  T8_ASSERT (forest->mpicomm == sc_MPI_COMM_NULL);
  T8_ASSERT (forest->cmesh == NULL);
  T8_ASSERT (forest->scheme == NULL);
  T8_ASSERT (forest->set_from == NULL);

  T8_ASSERT (set_from != NULL);

  forest->set_from = set_from;
  forest->from_method = T8_FOREST_FROM_COPY;
}

void
t8_forest_set_adapt (t8_forest_t forest, const t8_forest_t set_from)
{
  T8_ASSERT (forest != NULL);
  T8_ASSERT (!forest->constructed);
  T8_ASSERT (forest->mpicomm == sc_MPI_COMM_NULL);
  T8_ASSERT (forest->cmesh == NULL);
  T8_ASSERT (forest->scheme == NULL);
  T8_ASSERT (forest->set_from == NULL);

  T8_ASSERT (set_from != NULL);

  forest->set_from = set_from;
  forest->from_method = T8_FOREST_FROM_ADAPT;
}

void
t8_forest_set_partition (t8_forest_t forest, const t8_forest_t set_from,
                         int set_for_coarsening)
{
  T8_ASSERT (forest != NULL);
  T8_ASSERT (!forest->constructed);
  T8_ASSERT (forest->mpicomm == sc_MPI_COMM_NULL);
  T8_ASSERT (forest->cmesh == NULL);
  T8_ASSERT (forest->scheme == NULL);
  T8_ASSERT (forest->set_from == NULL);

  T8_ASSERT (set_from != NULL);

  forest->set_for_coarsening = set_for_coarsening;

  forest->set_from = set_from;
  forest->from_method = T8_FOREST_FROM_PARTITION;
}

void
t8_forest_construct (t8_forest_t forest)
{
  int                 mpiret;
  sc_MPI_Comm         comm_dup;

  T8_ASSERT (forest != NULL);
  T8_ASSERT (!forest->constructed);

  if (forest->set_from == NULL) {
    T8_ASSERT (forest->mpicomm != sc_MPI_COMM_NULL);
    T8_ASSERT (forest->cmesh != NULL);
    T8_ASSERT (forest->scheme != NULL);
    T8_ASSERT (forest->from_method == T8_FOREST_FROM_LAST);

    /* dup communicator if requested */
    if (forest->do_dup) {
      mpiret = sc_MPI_Comm_dup (forest->mpicomm, &comm_dup);
      SC_CHECK_MPI (mpiret);
      forest->mpicomm = comm_dup;
    }
  }
  else {
    T8_ASSERT (forest->mpicomm == sc_MPI_COMM_NULL);
    T8_ASSERT (forest->cmesh == NULL);
    T8_ASSERT (forest->scheme == NULL);
    T8_ASSERT (!forest->do_dup);
    T8_ASSERT (forest->from_method >= T8_FOREST_FROM_FIRST &&
               forest->from_method < T8_FOREST_FROM_LAST);

    /* TODO: optimize all this when forest->set_from has reference count one */

    /* we must prevent the case that set_from frees the source communicator */
    if (!forest->set_from->do_dup) {
      forest->mpicomm = forest->set_from->mpicomm;
    }
    else {
      mpiret = sc_MPI_Comm_dup (forest->set_from->mpicomm, &forest->mpicomm);
      SC_CHECK_MPI (mpiret);
    }
    forest->do_dup = forest->set_from->do_dup;

    /* increase reference count of cmesh and scheme from the input forest */
    t8_cmesh_ref (forest->cmesh = forest->set_from->cmesh);
    t8_scheme_ref (forest->scheme = forest->set_from->scheme);
    forest->dimension = forest->set_from->dimension;

    /* TODO: call adapt and partition subfunctions here */
    /* TODO: currently we can only handle copy */
    T8_ASSERT (forest->from_method == T8_FOREST_FROM_COPY);

    /* decrease reference count of input forest, possibly destroying it */
    t8_forest_unref (&forest->set_from);
  }

  /* query communicator anew */
  mpiret = sc_MPI_Comm_size (forest->mpicomm, &forest->mpisize);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_rank (forest->mpicomm, &forest->mpirank);
  SC_CHECK_MPI (mpiret);

  /* we do not need the set parameters anymore */
  forest->set_level = 0;
  forest->set_for_coarsening = 0;
  forest->set_from = NULL;
  forest->constructed = 1;
}

void
t8_forest_write_vtk (t8_forest_t forest, const char *filename)
{
  T8_ASSERT (forest != NULL);
  T8_ASSERT (forest->constructed);
}

static void
t8_forest_destroy (t8_forest_t * pforest)
{
  int                 mpiret;
  t8_forest_t         forest;

  T8_ASSERT (pforest != NULL);
  forest = *pforest;
  T8_ASSERT (forest != NULL);
  T8_ASSERT (forest->rc.refcount == 0);

  if (!forest->constructed) {
    if (forest->set_from != NULL) {
      /* in this case we have taken ownership and not released it yet */
      t8_forest_unref (&forest->set_from);
    }
  }
  else {
    T8_ASSERT (forest->set_from == NULL);
  }

  /* we have taken ownership on calling t8_forest_set_* */
  t8_scheme_unref (&forest->scheme);
  t8_cmesh_unref (&forest->cmesh);

  /* undup communicator if necessary */
  if (forest->do_dup) {
    mpiret = sc_MPI_Comm_free (&forest->mpicomm);
    SC_CHECK_MPI (mpiret);
  }

  T8_FREE (forest);
  *pforest = NULL;
}

void
t8_forest_ref (t8_forest_t forest)
{
  T8_ASSERT (forest != NULL);

  sc_refcount_ref (&forest->rc);
}

void
t8_forest_unref (t8_forest_t * pforest)
{
  t8_forest_t         forest;

  T8_ASSERT (pforest != NULL);
  forest = *pforest;
  T8_ASSERT (forest != NULL);

  if (sc_refcount_unref (&forest->rc)) {
    t8_forest_destroy (pforest);
  }
}
