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

/** \file t8_cmesh_vertex_conn_vertex_to_tree_c.hxx
 * Class to save data structure for vertex_to_tree_lists
 */

#ifndef T8_CMESH_VERTEX_CONN_VERTEX_TO_TREE_HXX
#define T8_CMESH_VERTEX_CONN_VERTEX_TO_TREE_HXX

#include <vector>
#include <unordered_map>
#include <t8_cmesh.h>

/*
 *  notes during development
 * 
 * This class stores the lookup
 * 
 * global_vertex_id -> List of (tree, tree_local_vertex) 
 * 
 * for a cmesh.
 * It is the opposite lookup as t8_cmesh_vertex_conn_tree_to_vertex_c
 * 
 * The global vertex ids must not be contiguous, that is, we have some set
 * 
 * {I_0 < I_1 < ...< I_N} of natural numbers corresponding to the N+1 vertices.
 * 
 * I_0 does not have to be 0 and I_N does not have to be N.
 * 
 * 
 * So we need lookup: I_i -> i
 *  store this in a hash table.
 * 
 * dattypes:
 * 
 * global id: t8_gloidx_t
 * (tree_id, tree_vertex): std::pair<t8_locidx_t, int> = TV_PAIR
 * List of (tree_id, tree_vertex): std::vector<PAIR> = TV_LIST
 * Table global_id -> TV_LIST: std::unordered_map<t8_gloidx_t, TV_LIST>
 * 
*/

typedef struct t8_cmesh_vertex_conn_vertex_to_tree_c
{
 public:
  t8_cmesh_vertex_conn_vertex_to_tree_c (): state (INITIALIZED)
  {
  }

  /* Variable type for (tree_id, tree_vertex_id) pair */
  using tree_vertex_pair = std::pair<t8_locidx_t, int>;

  /* list of tree vertex pairs, each global vertex id maps to 
    * such a list. */
  using tree_vertex_list = std::vector<tree_vertex_pair>;

  using vtt_storage_type = std::unordered_map<t8_gloidx_t, tree_vertex_list>;

  /* Setter functions */
  /* Given a cmesh, build up the vertex_to_tree.
   * \return: some error value to be specified.
   * The cmesh must not be committed, but all tree information and neighbor information must
   * have been set. 
   * Currently, \a cmesh has to be replicated. */
  void
  set_vertex_to_tree_list (const t8_cmesh_t cmesh);

  tree_vertex_list&
  get_tree_list_of_vertex (t8_gloidx_t global_vertex_id);

  /* Setter functions */
  /* A single value is added to the vertex_to_tree_list.
   * \a cmesh must be committed. */
  void
  add_vertex_to_tree (t8_cmesh_t cmesh, t8_gloidx_t global_vertex_id, t8_locidx_t ltreeid, int tree_vertex);

  /* Mark as ready for commit. Meaning that all 
   * global vertex ids have been added.
   * After commit, no vertex ids can be added anymore. */
  void
  commit (t8_cmesh_t cmesh);

  /**
   * @brief Check whether this instance is committed.
   * 
   * @return int True if committed. Thus all entries have been set.
   */
  int
  is_committed ();

  typedef vtt_storage_type::const_iterator const_iterator;

  const_iterator
  begin () const
  {
    return vertex_to_tree.begin ();
  }
  const_iterator
  end () const
  {
    return vertex_to_tree.end ();
  }

 private:
  /* For each global vertex id sort the list of
   * (tree_id, tree_vertex) pairs according to
   * tree_id and tree_vertex index.
   * Example: (1, 3), (0, 0), (1, 0)
   * becomes: (0, 0), (1, 0), (1, 3)
   */
  void
  sort_list_by_tree_id ();

  /* The actual data storage mapping global vertex ids to a list
   * local trees and tree vertices. */
  vtt_storage_type vertex_to_tree;

  enum { INITIALIZED, COMMITTED } state;

} t8_cmesh_vertex_conn_vertex_to_tree_c;

#endif /* !T8_CMESH_VERTEX_CONN_VERTEX_TO_TREE_HXX */
