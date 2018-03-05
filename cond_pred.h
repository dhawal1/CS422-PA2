/* Author: Jared Stark;   Created: Tue Jul 27 13:39:15 PDT 2004
 * Description: This file implements a conditional branch predictor.
*/

#ifndef COND_PRED_H_SEEN
#define COND_PRED_H_SEEN

#include <algorithm>
#include <cstddef>
#include <inttypes.h>

namespace cbp
{
    template <int LG2_SIZE>
    class COND_PRED
    {
      private:
        // not implemented
        explicit COND_PRED(const COND_PRED<LG2_SIZE>&);
        COND_PRED<LG2_SIZE>& operator=(const COND_PRED<LG2_SIZE>&);

        static const uint32_t HISTORY_MSB = (uint32_t(1) << (LG2_SIZE - 1));
        static const std::size_t SIZE = (std::size_t(1) << LG2_SIZE);
        static const std::size_t MASK = (SIZE - 1);
        uint32_t history;
        uint8_t table[SIZE];
        static std::size_t get_index(uint64_t vip, uint32_t history);

      public:
        COND_PRED(void) : history(0) { std::fill_n(table, SIZE, /* weakly taken */ uint8_t(2)); }
        // uses compiler generated destructor
    
        bool get_prediction(uint64_t vip) const;
        void train(uint64_t vip, bool taken);
    };

    template <int LG2_SIZE>
    inline std::size_t
    COND_PRED<LG2_SIZE>::get_index(uint64_t vip, uint32_t history)
    {
        return ((static_cast<std::size_t>(vip) ^ static_cast<std::size_t>(history)) & MASK);
    }

    template <int LG2_SIZE>
    bool
    COND_PRED<LG2_SIZE>::get_prediction(uint64_t vip) const
    {
        std::size_t index = get_index(vip, history);
        uint8_t counter = table[index];
        bool prediction = (counter >= 2);
        return prediction;   // true for taken, false for not taken
    }

    template <int LG2_SIZE>
    void
    COND_PRED<LG2_SIZE>::train(uint64_t vip, bool taken)
    {
        std::size_t index = get_index(vip, history);
        uint8_t counter = table[index];
        if (taken)
            counter += (counter != 3);
        else
            counter -= (counter != 0);
        table[index] = counter;

        history >>= 1;
        if (taken)
            history |= HISTORY_MSB;
    }
} // namespace cbp
    
#endif // COND_PRED_H_SEEN

