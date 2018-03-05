/* Author: Jared Stark;   Created: Mon Sep 22 21:53:52 PDT 2003
 * Description: This file implements an indirect branch predictor. 
*/

#ifndef INDIRECT_PRED_H_SEEN
#define INDIRECT_PRED_H_SEEN

#include <cstddef>
#include <inttypes.h>

namespace cbp
{
    template <int LG2_SIZE>
    class INDIRECT_PRED
    {
      private:
        // not implemented
        explicit INDIRECT_PRED(const INDIRECT_PRED<LG2_SIZE>&);
        INDIRECT_PRED<LG2_SIZE>& operator=(const INDIRECT_PRED<LG2_SIZE>&);
    
        static const std::size_t SIZE = (std::size_t(1) << LG2_SIZE);
        static const std::size_t MASK = (SIZE - 1);
        static const int SHIFT = (LG2_SIZE / 4);   // history shift count
        struct ENTRY
        {
            ENTRY(void) : history_tag(0), history(0), address(0) { }
            uint32_t history_tag;
            uint32_t history;
            uint64_t address;
        };
        ENTRY table[SIZE];
        static std::size_t get_history_index(uint64_t vip);
        static uint32_t get_history_tag(uint64_t vip);
        static std::size_t get_history_hash(uint32_t history);

      public:
        INDIRECT_PRED(void) { }
        // uses compiler generated destructor
     
        uint64_t get_prediction(uint64_t vip) const;
        void train(uint64_t vip, uint64_t address);
    };

    template <int LG2_SIZE>
    inline std::size_t
    INDIRECT_PRED<LG2_SIZE>::get_history_index(uint64_t vip)
    {
        return (static_cast<std::size_t>(vip) & MASK);
    }

    template <int LG2_SIZE>
    inline uint32_t
    INDIRECT_PRED<LG2_SIZE>::get_history_tag(uint64_t vip)
    {
        return static_cast<uint32_t>(vip >> LG2_SIZE);
    }

    template <int LG2_SIZE>
    inline std::size_t
    INDIRECT_PRED<LG2_SIZE>::get_history_hash(uint32_t history)
    {
        uint32_t p0 = ((history & uint32_t(0x000000ffUL)) >> (0 * (8 - SHIFT)));
        uint32_t p1 = ((history & uint32_t(0x0000ff00UL)) >> (1 * (8 - SHIFT)));
        uint32_t p2 = ((history & uint32_t(0x00ff0000UL)) >> (2 * (8 - SHIFT)));
        uint32_t p3 = ((history & uint32_t(0xff000000UL)) >> (3 * (8 - SHIFT)));
        return (static_cast<std::size_t>(p0 ^ p1 ^ p2 ^ p3) & MASK);
    }

    template <int LG2_SIZE>
    uint64_t
    INDIRECT_PRED<LG2_SIZE>::get_prediction(uint64_t vip) const
    {
        std::size_t history_index = get_history_index(vip);
        const ENTRY& history_entry = table[history_index];
        uint32_t history_tag = get_history_tag(vip);
        bool history_hit = (history_tag == history_entry.history_tag);
        uint32_t history = history_entry.history;

        if (!history_hit)
            return history_entry.address;

        std::size_t address_index = history_index;
        address_index ^= get_history_hash(history);
        const ENTRY& address_entry = table[address_index];

        return address_entry.address;
    }

    template <int LG2_SIZE>
    void
    INDIRECT_PRED<LG2_SIZE>::train(uint64_t vip, uint64_t address)
    {
        std::size_t history_index = get_history_index(vip);
        ENTRY& history_entry = table[history_index];
        uint32_t history_tag = get_history_tag(vip);
        bool history_hit = (history_tag == history_entry.history_tag);
        uint32_t history = history_entry.history;

        std::size_t address_index = history_index;
        address_index ^= get_history_hash(history);
        ENTRY& address_entry = table[address_index];

        uint32_t history_element;
        history_element = (static_cast<uint32_t>(address) & 0xff);

        if (history_hit) {
            history_entry.history = ((history << 8) | history_element);
            address_entry.address = address;
            return;
        }

        history_entry.history_tag = history_tag;
        history = ((history_element << 8) | history_element);
        history = ((history << 16) | history);
        history_entry.history = history;
        history_entry.address = address_entry.address;

        address_index = (history_index ^ get_history_hash(history));
        table[address_index].address = address;
    }
} // namespace cbp

#endif // INDIRECT_PRED_H_SEEN

