#include <gtest/gtest.h>
#include <t8_eclass.h>
#include <t8_schemes/t8_default/t8_default.hxx>
#include <vector>
#include <tuple>

// Define the user-defined operator++ for t8_eclass_t
t8_eclass_t&
operator++ (t8_eclass_t& eclass)
{
  eclass = static_cast<t8_eclass_t> (static_cast<int> (eclass) + 1);
  return eclass;
}

/** 
 * \class scheme_iterators
 * Class to iterate over all eclasses of all schemes and return the t8_scheme and the current t8_eclass_t.
 */
class scheme_iterators {
 public:
  /**
     * Initialize the iterator with a list of schemes.
     * \param [in] schemes The list of schemes to iterate over.
    */
  scheme_iterators (const std::vector<t8_scheme*>& schemes): schemes (schemes)
  {
  }

  /**
     * \struct Iterator
     * Iterator to iterate over all eclasses of all schemes.
    */
  struct Iterator
  {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::tuple<t8_scheme*, t8_eclass_t>;
    using pointer = std::tuple<t8_scheme*, t8_eclass_t>*;
    using reference = std::tuple<t8_scheme*, t8_eclass_t>&;

    /**
         * Constructor for the iterator.
         * \param [in] schemes The list of schemes to iterate over.
         * \param [in] is_end Flag to indicate if the iterator is at the end.
        */
    Iterator (const std::vector<t8_scheme*>& schemes, const bool is_end = false)
      : schemes (schemes), scheme_index (is_end ? schemes.size () : 0), eclass_index (t8_eclass_t (0))
    {
      if (!is_end && !schemes.empty ()) {
        eclass_count = schemes[scheme_index]->get_num_eclass_schemes ();
      }
    }

    /**
         * Dereference operator.
         * \return The current scheme and eclass.
        */
    std::tuple<t8_scheme*, t8_eclass_t>
    operator* () const
    {
      t8_scheme* current_scheme = schemes[scheme_index];
      t8_eclass_t current_eclass = eclass_index;
      return std::make_tuple (current_scheme, current_eclass);
    }

    /**
        *  Prefix increment operator to move the iterator to the next element. 
        * \return A reference to the updated iterator. 
        */
    Iterator&
    operator++ ()
    {
      if (static_cast<int> (++eclass_index) >= eclass_count) {
        eclass_index = t8_eclass_t (0);
        if (++scheme_index < schemes.size ()) {
          eclass_count = schemes[scheme_index]->get_num_eclass_schemes ();
        }
      }
      return *this;
    }

    /**
        * Inequality operator to compare two iterators.
        * \param [in] other Another iterator to compare with.
        * \return True if the iterators are not equal, false otherwise. 
        */
    bool
    operator!= (const Iterator& other) const
    {
      return scheme_index != other.scheme_index || eclass_index != other.eclass_index;
    }

   private:
    const std::vector<t8_scheme*>& schemes;
    size_t scheme_index;
    t8_eclass_t eclass_index;
    size_t eclass_count;
  };

  Iterator
  begin () const
  {
    return Iterator (schemes);
  }
  Iterator
  end () const
  {
    return Iterator (schemes, true);
  }

 private:
  const std::vector<t8_scheme*>& schemes;
};
