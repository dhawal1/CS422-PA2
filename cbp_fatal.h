/* Author: Jared Stark;   Created: Sun Oct  5 15:23:12 PDT 2003
 * Description: This file defines a macro that should be called for fatal
 * errors.
*/

#ifndef CBP_FATAL_H_SEEN
#define CBP_FATAL_H_SEEN

#include <cstdio>
#include <cstdlib>

#define CBP_FATAL(fmt, ...)                                     \
do {                                                            \
    std::fprintf(stderr, "fatal: " fmt " [%s:%s, line %d]\n",   \
        ## __VA_ARGS__, __FUNCTION__, __FILE__, __LINE__);      \
    std::exit(EXIT_FAILURE);                                    \
} while (0)

#endif // CBP_FATAL_H_SEEN

