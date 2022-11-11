#ifndef _UTILS_H_
#define _UTILS_H_

#ifdef DEBUGGING
    #define DEBUG(x) do { x } while (false)
#else
    #define DEBUG(x) do {   } while (false)
#endif

#define debug_assert(x) DEBUG(assert(x);)
#define debug_print

#endif
