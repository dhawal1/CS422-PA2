/* Author: Jared Stark;   Created: Tue Sep  9 06:02:16 PDT 2003
 * Description: This file implements a two-delta stride predictor.
*/

#ifndef STRIDE_PRED_H_SEEN
#define STRIDE_PRED_H_SEEN

#include <cstddef>
#include <inttypes.h>

namespace cbp 
{
    template <int LG2_SIZE>
    class STRIDE_PRED
    {
      private:
        // not implemented
        explicit STRIDE_PRED(const STRIDE_PRED<LG2_SIZE>&);
        STRIDE_PRED<LG2_SIZE>& operator=(const STRIDE_PRED<LG2_SIZE>&);
    
        static const std::size_t SIZE = (std::size_t(1) << LG2_SIZE);
        static const std::size_t MASK = (SIZE - 1); 
        struct ENTRY
        {
            ENTRY(void) : s1(0), s2(0), v(0) { }
            int32_t s1;   // stride 1
            int32_t s2;   // stride 2
            uint64_t v;   // value
        };
        ENTRY table[SIZE];
        static std::size_t get_index(uint64_t vip);

      public:
        STRIDE_PRED(void) { }
        // uses compiler generated destructor
     
        uint64_t get_prediction(uint64_t vip) const;
        void train(uint64_t vip, uint64_t v);
    };
    
    template <int LG2_SIZE>
    inline std::size_t
    STRIDE_PRED<LG2_SIZE>::get_index(uint64_t vip)
    {
        return (static_cast<std::size_t>(vip) & MASK);
    }
    
    template <int LG2_SIZE>
    uint64_t
    STRIDE_PRED<LG2_SIZE>::get_prediction(uint64_t vip) const
    {
        std::size_t index = get_index(vip);
        const ENTRY& entry = table[index];
        return (entry.v + entry.s2);
    }
    
    template <int LG2_SIZE>
    void
    STRIDE_PRED<LG2_SIZE>::train(uint64_t vip, uint64_t v)
    {
        std::size_t index = get_index(vip);
        ENTRY& entry = table[index];
        int32_t s;
        if (v > entry.v)
            s = static_cast<int32_t>(v - entry.v);
        else
            s = -static_cast<int32_t>(entry.v - v);
        if (entry.s1 == s)
            entry.s2 = s;
        entry.s1 = s;
        entry.v = v;
    }
} // namespace cbp

#endif // STRIDE_PRED_H_SEEN

