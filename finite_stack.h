/* Author: Jared Stark;   Created: Wed Sep 10 20:02:50 PDT 2003
 * Description: This file implements a finite size stack.
*/

#ifndef FINITE_STACK_H_SEEN
#define FINITE_STACK_H_SEEN
    
#include <algorithm>
#include <cstddef>
#include <inttypes.h>

namespace cbp
{
    template <std::size_t MAX_SIZE>
    class FINITE_STACK
    {
      private:
        // not implemented
        explicit FINITE_STACK(const FINITE_STACK<MAX_SIZE>&);
        FINITE_STACK<MAX_SIZE>& operator=(const FINITE_STACK<MAX_SIZE>&);
    
        std::size_t top_ptr;
        uint64_t stack[MAX_SIZE];

      public:
        FINITE_STACK(void) : top_ptr(0) { std::fill_n(stack, MAX_SIZE, 0); }
        // uses compiler generated destructor

        void push(uint64_t value);
        uint64_t top(void) const { return stack[top_ptr]; }
        void pop(void) { top_ptr = ((top_ptr - 1) % MAX_SIZE); } 
    };

    template <std::size_t MAX_SIZE>
    inline void
    FINITE_STACK<MAX_SIZE>::push(uint64_t value)
    {
        top_ptr = ((top_ptr + 1) % MAX_SIZE);
        stack[top_ptr] = value;
    }
} // namespace cbp

#endif // FINITE_STACK_H_SEEN

