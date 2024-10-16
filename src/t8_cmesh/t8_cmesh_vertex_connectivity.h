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

#ifndef T8_CMESH_VERTEX_CONNECTIVITY_H
#define T8_CMESH_VERTEX_CONNECTIVITY_H

typedef struct t8_cmesh_vertex_connectivity *t8_cmesh_vertex_connectivity_c;

void
t8_cmesh_set_global_vertices_of_tree (const t8_cmesh_t cmesh, const t8_gloidx_t global_tree,
                                      const t8_gloidx_t *global_tree_vertices, const int num_vertices);

t8_gloidx_t
t8_cmesh_get_num_global_vertices (const t8_cmesh_t cmesh);

t8_locidx_t
t8_cmesh_get_num_local_vertices (const t8_cmesh_t cmesh);

const t8_gloidx_t *
t8_cmesh_get_global_vertices_of_tree (const t8_cmesh_t cmesh, const t8_locidx_t local_tree, const int num_vertices);

const t8_gloidx_t
t8_cmesh_get_global_vertex_of_tree (const t8_cmesh_t cmesh, const t8_locidx_t local_tree, const int local_tree_vertex,
                                    const int num_vertices);

const int
t8_cmesh_get_num_trees_at_vertex (const t8_cmesh_t cmesh, t8_gloidx_t global_vertex);

#endif /* !T8_CMESH_VERTEX_CONNECTIVITY_H */
