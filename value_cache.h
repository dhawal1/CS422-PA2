/* Author: Jared Stark;   Created: Wed Sep 10 06:53:51 PDT 2003
 * Description: This file implements a value cache.
*/

#ifndef VALUE_CACHE_H_SEEN
#define VALUE_CACHE_H_SEEN

#include <algorithm>
#include <cstddef>
#include <inttypes.h>

namespace cbp
{
    template <int SET_BITS, int OFFSET_BITS>
    class VALUE_CACHE
    {
      private:
        // not implemented
        explicit VALUE_CACHE(const VALUE_CACHE<SET_BITS, OFFSET_BITS>&);
        VALUE_CACHE<SET_BITS, OFFSET_BITS>&
            operator=(const VALUE_CACHE<SET_BITS, OFFSET_BITS>&);
    
        static const std::size_t NUM_SETS = (std::size_t(1) << SET_BITS);
        static const std::size_t SET_MASK = (NUM_SETS - 1);
        static const uint64_t PAGE_MASK = (~uint64_t(0) << OFFSET_BITS);
        static const uint64_t OFFSET_MASK = ~PAGE_MASK;
        uint64_t cache[NUM_SETS];
        static std::size_t get_index(uint64_t val);

      public:
        VALUE_CACHE(void) { std::fill_n(cache, NUM_SETS, 0); }
        // uses compiler generated destructor

        typedef int id_type;   // value identifier
        static const id_type NOT_FOUND = id_type(-1);

        uint64_t operator[](id_type id) const;
        void insert(uint64_t val);
        id_type find(uint64_t val) const;
    };

    template <int SET_BITS, int OFFSET_BITS>
    inline std::size_t
    VALUE_CACHE<SET_BITS, OFFSET_BITS>::get_index(uint64_t val)
    {
        return (static_cast<std::size_t>(val >> OFFSET_BITS) & SET_MASK);
    }

    template <int SET_BITS, int OFFSET_BITS>
    uint64_t
    VALUE_CACHE<SET_BITS, OFFSET_BITS>::operator[](id_type id) const
    {
        std::size_t index = (id >> OFFSET_BITS);
        id_type offset = (id & static_cast<id_type>(OFFSET_MASK));
        return (cache[index] + offset);
    }

    template <int SET_BITS, int OFFSET_BITS>
    void
    VALUE_CACHE<SET_BITS, OFFSET_BITS>::insert(uint64_t val)
    {
        std::size_t index = get_index(val);
        uint64_t tag = (val & PAGE_MASK);
        cache[index] = tag;
    }
    
    template <int SET_BITS, int OFFSET_BITS>
    typename VALUE_CACHE<SET_BITS, OFFSET_BITS>::id_type
    VALUE_CACHE<SET_BITS, OFFSET_BITS>::find(uint64_t val) const
    {
        std::size_t index = get_index(val);
        uint64_t tag = (val & PAGE_MASK);
        return ((cache[index] == tag)
            ? /* found */ ((index << OFFSET_BITS) + (val - tag))
            : NOT_FOUND);
    }
} // namespace cbp

#endif // VALUE_CACHE_H_SEEN

