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

/** \file t8_cmesh_vertex_connectivity.hxx
 * We define classes and interfaces for a global vertex enumeration
 * of a cmesh.
 */

#pragma once

#include <memory>
#include <t8_cmesh.h>
#include <t8_cmesh/t8_cmesh_vertex_conn_vertex_to_tree.hxx>
#include <t8_cmesh/t8_cmesh_vertex_conn_tree_to_vertex.hxx>

struct t8_cmesh_vertex_connectivity
{
 public:
  /**
   * Constructor.
   */
  t8_cmesh_vertex_connectivity (): global_number_of_vertices (0), local_number_of_vertices (0) {};

  /**
   * Destructor.
   */
  ~t8_cmesh_vertex_connectivity () {};

  /* Given a cmesh, build up the vertex_to_tree and tree_to_vertex members.
   * \return: some error value to be specified.
   * On error, \state will be set to ERROR. 
   * The cmesh must not be committed, but all tree information and neighbor information must
   * have been set. 
   * Currently, \a cmesh has to be replicated. */
  void
  build (const t8_cmesh_t cmesh);

  /* Setter functions */

  /** Set all global vertex ids of a local tree. 
   * \param[in] cmesh The considered cmesh
   * \param[in] local_tree A local tree id of \a cmesh
   * \param[in] global_vertex_id The ids of the global vertices in order of \a local_tree's vertices.
   * \param[in] num_vertices Must match the number of vertices of \a local_tree
   * 
   * \note \a cmesh must not be committed.
  */
  inline void
  set_global_vertex_ids_of_tree_vertices (const t8_cmesh_t cmesh, const t8_gloidx_t global_tree,
                                          const t8_gloidx_t *global_tree_vertices, const int num_vertices)
  {
    return tree_to_vertex.set_global_vertex_ids_of_tree_vertices (cmesh, global_tree, global_tree_vertices,
                                                                  num_vertices);
  }

  inline void
  set_global_vertices_of_tree (const t8_cmesh_t cmesh, const t8_gloidx_t global_tree,
                               const t8_gloidx_t *global_tree_vertices, const int num_vertices)
  {
    T8_ASSERT (t8_cmesh_is_initialized (cmesh));
    set_global_vertex_ids_of_tree_vertices (cmesh, global_tree, global_tree_vertices, num_vertices);
  }

  enum t8_cmesh_vertex_connectivity_state_t {
    INITIALIZED,
    VERTEX_TO_TREE_VALID,
    TREE_TO_VERTEX_VALID,
    VTT_AND_TTV_VALID
  };

  /* Build vertex_to_tree from existing tree_to_vertex */
  void
  build_vertex_to_tree (const t8_cmesh_t cmesh)
  {
    vertex_to_tree.build_from_ttv (cmesh, tree_to_vertex);
    global_number_of_vertices = vertex_to_tree.vertex_to_tree.size ();
    state = VTT_AND_TTV_VALID;
  }

  /* Getter functions */

  /* Get the global number of vertices in the cmesh */
  inline const t8_gloidx_t
  get_global_number_of_vertices ()
  {
    return global_number_of_vertices;
  }

  /* Get the process local number of vertices in the cmesh */
  inline const t8_gloidx_t
  get_local_number_of_vertices ()
  {
    /* TODO: Change this after the global vertex connectivity is working partitioned. */
    return local_number_of_vertices = global_number_of_vertices;
  }

  inline t8_gloidx_t
  get_num_global_vertices (const t8_cmesh_t cmesh)
  {
    T8_ASSERT (t8_cmesh_is_committed (cmesh));
    return get_global_number_of_vertices ();
  }

  inline t8_locidx_t
  get_num_local_vertices (const t8_cmesh_t cmesh)
  {
    T8_ASSERT (t8_cmesh_is_committed (cmesh));
    return get_local_number_of_vertices ();
  }

  /* Return the state of the connectivity */
  inline const t8_cmesh_vertex_connectivity_state_t
  get_state ();

  /* Given a global vertex id, return the list of trees that share this vertex */
  inline const t8_cmesh_vertex_conn_vertex_to_tree::vtt_storage_type &
  vertex_to_trees (t8_gloidx_t vertex_id);

  /* Given a local tree (or ghost) return the list of global vertices
   * this tree has (in order of its local vertices). */
  inline const t8_locidx_t *
  tree_to_vertices (t8_locidx_t ltree);

  /* Given a local tree (or ghost) and a local vertex of that tree
   * return the global vertex id of that vertex */
  inline const t8_gloidx_t
  treevertex_to_vertex (t8_locidx_t ltree, t8_locidx_t ltree_vertex);

  /* Get the global vertex indices of a tree in its local vertex order */
  inline const t8_gloidx_t *
  get_global_vertices (t8_cmesh_t cmesh, t8_locidx_t local_number_of_vertices, int num_vertices)
  {
    return tree_to_vertex.get_global_vertices (cmesh, local_number_of_vertices, num_vertices);
  }

  inline const t8_gloidx_t *
  get_global_vertices_of_tree (const t8_cmesh_t cmesh, const t8_locidx_t local_tree, const int num_vertices)
  {
    T8_ASSERT (t8_cmesh_is_committed (cmesh));
    return get_global_vertices (cmesh, local_tree, num_vertices);
  }

  inline const t8_gloidx_t
  get_global_vertex_of_tree (const t8_cmesh_t cmesh, const t8_locidx_t local_tree, const int local_tree_vertex,
                             const int num_vertices)
  {
    T8_ASSERT (t8_cmesh_is_committed (cmesh));
    const t8_gloidx_t *vertices_of_tree = get_global_vertices_of_tree (cmesh, local_tree, num_vertices);
    return vertices_of_tree[local_tree_vertex];
  }

  /* Get the global vertex index of a local vertex of a tree */
  inline const t8_cmesh_vertex_conn_vertex_to_tree::tree_vertex_list &
  get_tree_list_of_vertex (t8_gloidx_t global_vertex_id)
  {
    return vertex_to_tree.get_tree_list_of_vertex (global_vertex_id);
  }

  inline const t8_cmesh_vertex_conn_vertex_to_tree::tree_vertex_list &
  get_vertex_to_tree_list (const t8_cmesh_t cmesh, const t8_gloidx_t global_vertex)
  {
    T8_ASSERT (t8_cmesh_is_committed (cmesh));

    return get_tree_list_of_vertex (global_vertex);
  }

  /* Note, if a tree is contained multiple times it is counted as multiple entries.
   * Example: A quad where all 4 vertices map to a single global vertex. This function will return 4. */
  inline const int
  get_num_trees_at_vertex (const t8_cmesh_t cmesh, t8_gloidx_t global_vertex)
  {
    T8_ASSERT (t8_cmesh_is_committed (cmesh));

    return get_tree_list_of_vertex (global_vertex).size ();
  }

  /* Get the current state of the vertex_to_tree instance */
  inline const int
  get_vertex_to_tree_state ()
  {
    return vertex_to_tree.get_state ();
  }

  /* Get the current state of the tree_to_vertex instance */
  inline const int
  get_tree_to_vertex_state ()
  {
    return tree_to_vertex.get_state ();
  }

 private:
  t8_cmesh_vertex_connectivity_state_t state;

  t8_gloidx_t global_number_of_vertices;

  /* Currently not used/equal to global number of vertices */
  t8_locidx_t local_number_of_vertices;

  t8_cmesh_vertex_conn_vertex_to_tree vertex_to_tree;

  t8_cmesh_vertex_conn_tree_to_vertex tree_to_vertex;
};
