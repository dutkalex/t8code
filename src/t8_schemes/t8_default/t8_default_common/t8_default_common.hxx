/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

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

/** \file t8_default_common.hxx
 * We provide some functions that are useful across element classes.
 */

#pragma once

#include <t8_element.hxx>
#include <t8_schemes/t8_crtp.hxx>
#include <sc_functions.h>

/* Macro to check whether a pointer (VAR) to a base class, comes from an
 * implementation of a child class (TYPE). */
#define T8_COMMON_IS_TYPE(VAR, TYPE) ((dynamic_cast<TYPE> (VAR)) != NULL)

/** This class independent function assumes an sc_mempool_t as context.
 * It is suitable as the elem_new callback in \ref t8_eclass_scheme_t.
 * We assume that the mempool has been created with the correct element size.
 * \param [in,out] ts_context   An element is allocated in this sc_mempool_t.
 * \param [in]     length       Non-negative number of elements to allocate.
 * \param [in,out] elem         Array of correct size whose members are filled.
 */
inline static void
t8_default_mempool_alloc (sc_mempool_t *ts_context, int length, t8_element_t **elem)
{
  int i;

  T8_ASSERT (ts_context != NULL);
  T8_ASSERT (0 <= length);
  T8_ASSERT (elem != NULL);

  for (i = 0; i < length; ++i) {
    elem[i] = (t8_element_t *) sc_mempool_alloc (ts_context);
  }
}

/** This class independent function assumes an sc_mempool_t as context.
 * It is suitable as the elem_destroy callback in \ref t8_eclass_scheme_t.
 * We assume that the mempool has been created with the correct element size.
 * \param [in,out] ts_context   An element is returned to this sc_mempool_t.
 * \param [in]     length       Non-negative number of elements to destroy.
 * \param [in,out] elem         Array whose members are returned to the mempool.
 */
inline static void
t8_default_mempool_free (sc_mempool_t *ts_context, int length, t8_element_t **elem)
{
  int i;

  T8_ASSERT (ts_context != NULL);
  T8_ASSERT (0 <= length);
  T8_ASSERT (elem != NULL);

  for (i = 0; i < length; ++i) {
    sc_mempool_free (ts_context, elem[i]);
  }
}

/* Given an element's level and dimension, return the number of leaves it
 * produces at a given uniform refinement level */
static inline t8_gloidx_t
count_leaves_from_level (int element_level, int refinement_level, int dimension)
{
  return element_level > refinement_level ? 0 : sc_intpow64 (2, dimension * (refinement_level - element_level));
}

template <class TUnderlyingEclass_Scheme>
class t8_default_scheme_common: public t8_crtp<TUnderlyingEclass_Scheme> {
 private:
  friend TUnderlyingEclass_Scheme;
  /** Private constructor which can only be used by derived schemes.
   * \param [in] tree_class The tree class of this element scheme.
   * \param [in] elem_size  The size of the elements this scheme holds.
  */
  t8_default_scheme_common (const t8_eclass_t tree_class, const size_t elem_size)
    : element_size (elem_size), eclass (tree_class)
  {
    ts_context = sc_mempool_new (elem_size);
  };

 protected:
  const size_t element_size; /**< The size in bytes of an element of class \a eclass */
  void *ts_context;          /**< Anonymous implementation context. */

 public:
  const t8_eclass_t eclass; /**< The tree class */

  /** Destructor for all default schemes */
  ~t8_default_scheme_common ()
  {
    T8_ASSERT (ts_context != NULL);
    SC_ASSERT (((sc_mempool_t *) ts_context)->elem_count == 0);
    sc_mempool_destroy ((sc_mempool_t *) ts_context);
  }

  /** Compute the number of corners of a given element. */
  int
  element_get_num_corners (const t8_element_t *elem) const
  {
    /* use the lookup table of the eclasses.
     * Pyramids should implement their own version of this function. */
    return t8_eclass_num_vertices[eclass];
  }

  /** Allocate space for a bunch of elements. */
  void
  element_new (int length, t8_element_t **elem) const
  {
    t8_default_mempool_alloc ((sc_mempool_t *) ts_context, length, elem);
  }

  /** Deallocate space for a bunch of elements. */
  void
  element_destroy (int length, t8_element_t **elem) const
  {
    t8_default_mempool_free ((sc_mempool_t *) ts_context, length, elem);
  }

  void
  element_deinit (int length, t8_element_t *elem) const
  {
  }

  /** Return the shape of an element */
  t8_element_shape_t
  element_get_shape (const t8_element_t *elem) const
  {
    /* use the lookup table of the eclasses.
     * Pyramids should implement their own version of this function. */
    return eclass;
  }

  /** Count how many leaf descendants of a given uniform level an element would produce.
   * \param [in] t     The element to be checked.
   * \param [in] level A refinement level.
   * \return Suppose \a t is uniformly refined up to level \a level. The return value
   * is the resulting number of elements (of the given level).
   * Each default element (except pyramids) refines into 2^{dim * (level - level(t))}
   * children.
   */
  t8_gloidx_t
  element_count_leaves (const t8_element_t *t, int level) const
  {
    int element_level = this->underlying ().element_get_level (t);
    t8_element_shape_t element_shape;
    int dim = t8_eclass_to_dimension[eclass];
    element_shape = element_get_shape (t);
    if (element_shape == T8_ECLASS_PYRAMID) {
      int level_diff = level - element_level;
      return element_level > level ? 0 : 2 * sc_intpow64 (8, level_diff) - sc_intpow64 (6, level_diff);
    }
    return count_leaves_from_level (element_level, level, dim);
  }

  /** Compute the number of siblings of an element. That is the number of 
   * Children of its parent.
   * \param [in] elem The element.
   * \return          The number of siblings of \a element.
   * Note that this number is >= 1, since we count the element itself as a sibling.
   */
  int
  element_get_num_siblings (const t8_element_t *elem) const
  {
    const int dim = t8_eclass_to_dimension[eclass];
    T8_ASSERT (eclass != T8_ECLASS_PYRAMID);
    return sc_intpow (2, dim);
  }

  /** Count how many leaf descendants of a given uniform level the root element will produce.
   * \param [in] level A refinement level.
   * \return The value of \ref t8_element_count_leaves if the input element
   *      is the root (level 0) element.
   */
  t8_gloidx_t
  count_leaves_from_root (int level) const
  {
    if (eclass == T8_ECLASS_PYRAMID) {
      return 2 * sc_intpow64u (8, level) - sc_intpow64u (6, level);
    }
    int dim = t8_eclass_to_dimension[eclass];
    return count_leaves_from_level (0, level, dim);
  }

#if T8_ENABLE_DEBUG
  void
  element_debug_print (const t8_element_t *elem) const
  {
    char debug_string[BUFSIZ];
    this->underlying ().element_to_string (elem, debug_string, BUFSIZ);
    t8_debugf ("%s\n", debug_string);
  }
#endif
};
