/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element types in parallel.

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

#include <gtest/gtest.h>
#include <unistd.h>             /* Needed to check for file access */
#include <t8.h>
#include <t8_eclass.h>
#include <t8_cmesh.h>
#include <t8_cmesh_readmshfile.h>
#include "t8_cmesh/t8_cmesh_trees.h"

/* In this file we test the msh file (gmsh) reader of the cmesh.
 * Currently, we support version 2 and 4 ascii.
 * We read a mesh file and check whether the constructed cmesh is correct.
 * We also try to read version 2 binary and version 4 binary
 * formats. All are not supported and we expect the reader to catch this.
 */

/* *INDENT-OFF* */
TEST(t8_cmesh_readmshfile, test_msh_file_vers2_ascii){
    
    const char  fileprefix[BUFSIZ - 4] = "test/testfiles/test_msh_file_vers2_ascii";
    char        filename[BUFSIZ];

    snprintf (filename, BUFSIZ, "%s.msh", fileprefix);
    
    EXPECT_FALSE(access (filename, R_OK));
    
    t8_cmesh_t cmesh = t8_cmesh_from_msh_file (fileprefix, 1, sc_MPI_COMM_WORLD, 2, 0, 0);
    
    EXPECT_TRUE(cmesh != NULL);
    
    int                 checkval;
    int                 read_node = 1;
    int                 check_neigh_elem = 1;
    double             *vertices;
    t8_locidx_t         ltree_id;
    t8_locidx_t         lnum_trees;
    t8_eclass_t         tree_class;

  /* Description of the properties of the example msh-files. */
  const int           number_elements = 4;
  const t8_eclass_t   elem_type = T8_ECLASS_TRIANGLE;
  
  int vertex[6][2] = {
                       {0, 0},
                       {2, 0},
                       {4, 0},
                       {1, 2},
                       {3, 2},
                       {2, 4} };

  int elements[4][3] = {
                        {0, 1, 3},
                        {1, 4, 3},
                        {1, 2, 4},
                        {3, 4, 5} };

  int face_neigh_elem[4][3] = {
                                {1, -1,-1},
                                {3, 0, 2},
                                {-1, 1, -1},
                                {-1, -1, 1} };

  if (cmesh == NULL) {
    /* If the cmesh is NULL. */
    checkval = 0;
  }
  else {
    /* Checks if the cmesh was comitted. */
    EXPECT_TRUE(t8_cmesh_is_committed (cmesh));
    /* Checks for face consistency. */
    EXPECT_TRUE(t8_cmesh_trees_is_face_consistend (cmesh, cmesh->trees));
    
    /* Checks if the number of elements was read correctly. */
    EXPECT_EQ(t8_cmesh_get_num_trees (cmesh), number_elements); 
    
    /* Number of local trees. */
    lnum_trees = t8_cmesh_get_num_local_trees (cmesh);
    /* Iterate through the local elements and check if they were read properly. */
    /* All trees should be local to the master rank. */
    for (t8_locidx_t ltree_it = 0; ltree_it < lnum_trees; ltree_it++) {
      tree_class = t8_cmesh_get_tree_class (cmesh, ltree_it);
      EXPECT_FALSE(t8_eclass_compare (tree_class, elem_type));
      
      /* Get pointer to the vertices of the tree. */
      vertices = t8_cmesh_get_tree_vertices (cmesh, ltree_it);
      /* Checking the msh-files elements and nodes. */
      for (int i = 0; i < 3; i++) {
        /* Checks if x and y coordinate of the nodes are not read correctly. */
        if (!((vertex[elements[ltree_it][i]][0] == (int) vertices[3 * i])
              && (vertex[elements[ltree_it][i]][1] ==
                  (int) vertices[(3 * i) + 1]))) {
          read_node = 0;
          EXPECT_TRUE(read_node);
          /* Return error, if the nodes are not read correctly. */
          checkval = -1;
        }
        /* Checks whether the face neighbor elements are not read correctly. */
        ltree_id =
          t8_cmesh_get_face_neighbor (cmesh, ltree_it, i, NULL, NULL);
        if (!(ltree_id == face_neigh_elem[ltree_it][i])) {
          check_neigh_elem = 0;
          EXPECT_TRUE(check_neigh_elem);
          /* Return error, if the face neighbor elements are not read correctly. */
          checkval = -1;
        }
      }
    }
    /* If the checks were performed correctly. */
    checkval = 1;
  }
  
  EXPECT_EQ(checkval, 1);

  /* The cmesh was read sucessfully and we need to destroy it. */
  t8_cmesh_destroy (&cmesh);
}

TEST(t8_cmesh_readmshfile, test_msh_file_vers4_ascii){
    
    const char  fileprefix[BUFSIZ - 4] = "test/testfiles/test_msh_file_vers4_ascii";
    char        filename[BUFSIZ];
    
    snprintf (filename, BUFSIZ, "%s.msh", fileprefix);

    EXPECT_FALSE(access (filename, R_OK));
    t8_cmesh_t cmesh = t8_cmesh_from_msh_file (fileprefix, 1, sc_MPI_COMM_WORLD, 2, 0, 0);
        
    EXPECT_TRUE(cmesh != NULL);
    
    int                 checkval;
    int                 read_node = 1;
    int                 check_neigh_elem = 1;
    double             *vertices;
    t8_locidx_t         ltree_id;
    t8_locidx_t         lnum_trees;
    t8_eclass_t         tree_class;

  /* Description of the properties of the example msh-files. */
  const int           number_elements = 4;
  const t8_eclass_t   elem_type = T8_ECLASS_TRIANGLE;
  /* *INDENT-OFF* */
  int vertex[6][2] = {
                       {0, 0},
                       {2, 0},
                       {4, 0},
                       {1, 2},
                       {3, 2},
                       {2, 4} };

  int elements[4][3] = {
                        {0, 1, 3},
                        {1, 4, 3},
                        {1, 2, 4},
                        {3, 4, 5} };

  int face_neigh_elem[4][3] = {
                                {1, -1,-1},
                                {3, 0, 2},
                                {-1, 1, -1},
                                {-1, -1, 1} };

  if (cmesh == NULL) {
    /* If the cmesh is NULL. */
    checkval = 0;
  }
  else {
    /* Checks if the cmesh was comitted. */
    EXPECT_TRUE(t8_cmesh_is_committed (cmesh));
    /* Checks for face consistency. */
    EXPECT_TRUE(t8_cmesh_trees_is_face_consistend (cmesh, cmesh->trees));
    
    /* Checks if the number of elements was read correctly. */
    EXPECT_EQ(t8_cmesh_get_num_trees (cmesh), number_elements); 
    
    /* Number of local trees. */
    lnum_trees = t8_cmesh_get_num_local_trees (cmesh);
    /* Iterate through the local elements and check if they were read properly. */
    /* All trees should be local to the master rank. */
    for (t8_locidx_t ltree_it = 0; ltree_it < lnum_trees; ltree_it++) {
      tree_class = t8_cmesh_get_tree_class (cmesh, ltree_it);
      EXPECT_FALSE(t8_eclass_compare (tree_class, elem_type));
      
      /* Get pointer to the vertices of the tree. */
      vertices = t8_cmesh_get_tree_vertices (cmesh, ltree_it);
      /* Checking the msh-files elements and nodes. */
      for (int i = 0; i < 3; i++) {
        /* Checks if x and y coordinate of the nodes are not read correctly. */
        if (!((vertex[elements[ltree_it][i]][0] == (int) vertices[3 * i])
              && (vertex[elements[ltree_it][i]][1] ==
                  (int) vertices[(3 * i) + 1]))) {
          read_node = 0;
          EXPECT_TRUE(read_node);
          /* Return error, if the nodes are not read correctly. */
          checkval = -1;
        }
        /* Checks whether the face neighbor elements are not read correctly. */
        ltree_id =
          t8_cmesh_get_face_neighbor (cmesh, ltree_it, i, NULL, NULL);
        if (!(ltree_id == face_neigh_elem[ltree_it][i])) {
          check_neigh_elem = 0;
          EXPECT_TRUE(check_neigh_elem);
          /* Return error, if the face neighbor elements are not read correctly. */
          checkval = -1;
        }
      }
    }
    /* If the checks were performed correctly. */
    checkval = 1;
  }
  
  EXPECT_EQ(checkval, 1);

  /* The cmesh was read sucessfully and we need to destroy it. */
  t8_cmesh_destroy (&cmesh);
}


TEST(t8_cmesh_readmshfile, test_msh_file_vers2_bin){
    
    const char fileprefix[BUFSIZ - 4] = "test/testfiles/test_msh_file_vers2_bin";
    char      filename[BUFSIZ];

    snprintf (filename, BUFSIZ, "%s.msh", fileprefix);

    EXPECT_FALSE(access (filename, R_OK));
    t8_cmesh_t cmesh = t8_cmesh_from_msh_file (fileprefix, 1, sc_MPI_COMM_WORLD, 2, 0, 0);

    EXPECT_TRUE(cmesh == NULL);

    int                 checkval;
    int                 read_node = 1;
    int                 check_neigh_elem = 1;
    double             *vertices;
    t8_locidx_t         ltree_id;
    t8_locidx_t         lnum_trees;
    t8_eclass_t         tree_class;

  /* Description of the properties of the example msh-files. */
  const int           number_elements = 4;
  const t8_eclass_t   elem_type = T8_ECLASS_TRIANGLE;
  /* *INDENT-OFF* */
  int vertex[6][2] = {
                       {0, 0},
                       {2, 0},
                       {4, 0},
                       {1, 2},
                       {3, 2},
                       {2, 4} };

  int elements[4][3] = {
                        {0, 1, 3},
                        {1, 4, 3},
                        {1, 2, 4},
                        {3, 4, 5} };

  int face_neigh_elem[4][3] = {
                                {1, -1,-1},
                                {3, 0, 2},
                                {-1, 1, -1},
                                {-1, -1, 1} };

  if (cmesh == NULL) {
    /* If the cmesh is NULL. */
    checkval = 0;
  }
  else {
    /* Checks if the cmesh was comitted. */
    EXPECT_TRUE(t8_cmesh_is_committed (cmesh));
    /* Checks for face consistency. */
    EXPECT_TRUE(t8_cmesh_trees_is_face_consistend (cmesh, cmesh->trees));
    
    /* Checks if the number of elements was read correctly. */
    EXPECT_EQ(t8_cmesh_get_num_trees (cmesh), number_elements); 
    
    /* Number of local trees. */
    lnum_trees = t8_cmesh_get_num_local_trees (cmesh);
    /* Iterate through the local elements and check if they were read properly. */
    /* All trees should be local to the master rank. */
    for (t8_locidx_t ltree_it = 0; ltree_it < lnum_trees; ltree_it++) {
      tree_class = t8_cmesh_get_tree_class (cmesh, ltree_it);
      EXPECT_FALSE(t8_eclass_compare (tree_class, elem_type));
      
      /* Get pointer to the vertices of the tree. */
      vertices = t8_cmesh_get_tree_vertices (cmesh, ltree_it);
      /* Checking the msh-files elements and nodes. */
      for (int i = 0; i < 3; i++) {
        /* Checks if x and y coordinate of the nodes are not read correctly. */
        if (!((vertex[elements[ltree_it][i]][0] == (int) vertices[3 * i])
              && (vertex[elements[ltree_it][i]][1] ==
                  (int) vertices[(3 * i) + 1]))) {
          read_node = 0;
          EXPECT_TRUE(read_node);
          /* Return error, if the nodes are not read correctly. */
          checkval = -1;
        }
        /* Checks whether the face neighbor elements are not read correctly. */
        ltree_id =
          t8_cmesh_get_face_neighbor (cmesh, ltree_it, i, NULL, NULL);
        if (!(ltree_id == face_neigh_elem[ltree_it][i])) {
          check_neigh_elem = 0;
          EXPECT_TRUE(check_neigh_elem);
          /* Return error, if the face neighbor elements are not read correctly. */
          checkval = -1;
        }
      }
    }
    /* If the checks were performed correctly. */
    checkval = 1;
  }
  EXPECT_EQ(checkval, 0);
}

TEST(t8_cmesh_readmshfile, test_msh_file_vers4_bin){
    
    const char fileprefix[BUFSIZ - 4] = "test/testfiles/test_msh_file_vers4_bin";
    char      filename[BUFSIZ];

    snprintf (filename, BUFSIZ, "%s.msh", fileprefix);

    EXPECT_FALSE(access (filename, R_OK));
    t8_cmesh_t cmesh = t8_cmesh_from_msh_file (fileprefix, 1, sc_MPI_COMM_WORLD, 2, 0, 0);

    EXPECT_TRUE(cmesh == NULL);

    int                 checkval;
    int                 read_node = 1;
    int                 check_neigh_elem = 1;
    double             *vertices;
    t8_locidx_t         ltree_id;
    t8_locidx_t         lnum_trees;
    t8_eclass_t         tree_class;

  /* Description of the properties of the example msh-files. */
  const int           number_elements = 4;
  const t8_eclass_t   elem_type = T8_ECLASS_TRIANGLE;
  /* *INDENT-OFF* */
  int vertex[6][2] = {
                       {0, 0},
                       {2, 0},
                       {4, 0},
                       {1, 2},
                       {3, 2},
                       {2, 4} };

  int elements[4][3] = {
                        {0, 1, 3},
                        {1, 4, 3},
                        {1, 2, 4},
                        {3, 4, 5} };

  int face_neigh_elem[4][3] = {
                                {1, -1,-1},
                                {3, 0, 2},
                                {-1, 1, -1},
                                {-1, -1, 1} };

  if (cmesh == NULL) {
    /* If the cmesh is NULL. */
    checkval = 0;
  }
  else {
    /* Checks if the cmesh was comitted. */
    EXPECT_TRUE(t8_cmesh_is_committed (cmesh));
    /* Checks for face consistency. */
    EXPECT_TRUE(t8_cmesh_trees_is_face_consistend (cmesh, cmesh->trees));
    
    /* Checks if the number of elements was read correctly. */
    EXPECT_EQ(t8_cmesh_get_num_trees (cmesh), number_elements); 
    
    /* Number of local trees. */
    lnum_trees = t8_cmesh_get_num_local_trees (cmesh);
    /* Iterate through the local elements and check if they were read properly. */
    /* All trees should be local to the master rank. */
    for (t8_locidx_t ltree_it = 0; ltree_it < lnum_trees; ltree_it++) {
      tree_class = t8_cmesh_get_tree_class (cmesh, ltree_it);
      EXPECT_FALSE(t8_eclass_compare (tree_class, elem_type));
      
      /* Get pointer to the vertices of the tree. */
      vertices = t8_cmesh_get_tree_vertices (cmesh, ltree_it);
      /* Checking the msh-files elements and nodes. */
      for (int i = 0; i < 3; i++) {
        /* Checks if x and y coordinate of the nodes are not read correctly. */
        if (!((vertex[elements[ltree_it][i]][0] == (int) vertices[3 * i])
              && (vertex[elements[ltree_it][i]][1] ==
                  (int) vertices[(3 * i) + 1]))) {
          read_node = 0;
          EXPECT_TRUE(read_node);
          /* Return error, if the nodes are not read correctly. */
          checkval = -1;
        }
        /* Checks whether the face neighbor elements are not read correctly. */
        ltree_id =
          t8_cmesh_get_face_neighbor (cmesh, ltree_it, i, NULL, NULL);
        if (!(ltree_id == face_neigh_elem[ltree_it][i])) {
          check_neigh_elem = 0;
          EXPECT_TRUE(check_neigh_elem);
          /* Return error, if the face neighbor elements are not read correctly. */
          checkval = -1;
        }
      }
    }
    /* If the checks were performed correctly. */
    checkval = 1;
  }
  EXPECT_EQ(checkval, 0);
}
/* *INDENT-ON* */