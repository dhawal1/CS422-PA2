/* Author: Jared Stark;   Created: Fri Nov 14 21:06:34 PST 2003
 * Description: This file defines a macro that should be called for assertions.
*/

#ifndef CBP_ASSERT_H_SEEN
#define CBP_ASSERT_H_SEEN

#include "cbp_fatal.h"

#define CBP_ASSERT(expression)                          \
do {                                                    \
    if (0 == (expression))                              \
        CBP_FATAL("assertion failed: " # expression);   \
} while (0)

#endif // CBP_ASSERT_H_SEEN

