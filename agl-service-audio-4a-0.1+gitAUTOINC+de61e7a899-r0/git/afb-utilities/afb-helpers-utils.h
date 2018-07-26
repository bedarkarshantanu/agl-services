#ifndef AFB_HELPERS_UTILS_H
#define AFB_HELPERS_UTILS_H

#include <limits.h>

#define xstr(s) str(s)
#define str(s) #s

/* with gcc >= 7.2.0, this macro is useful when printing an int with snprintf:
 *
 * char[INT_STR_MAX]; // smaller value leads to a warning
 * snprintf(targetS, sizeof (targetS), "%d", target);
 * */

#define INT_STR_MAX sizeof(xstr(INT_MAX))

#endif /* AFB_HELPERS_UTILS_H */
