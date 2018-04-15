#ifndef DEBUG_H

#ifdef DEBUG_ON

#include <cstring>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define DEBUG(message, ...) { \
        char buffer[1 << 10]; \
        sprintf(buffer, message, ##__VA_ARGS__); \
        printf("%s %s: %s\n", __FILENAME__, __PRETTY_FUNCTION__, buffer); \
    }

#else 

#define DEBUG(...)

#endif

#endif