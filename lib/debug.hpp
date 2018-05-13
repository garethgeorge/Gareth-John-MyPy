#ifndef DEBUG_H
#define DEBUG_H 

#ifdef DEBUG_ON

#include <cstring>
#include <iostream>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define DEBUG(message, ...) { \
    char buffer[1 << 10]; \
    sprintf(buffer, message, ##__VA_ARGS__); \
    fprintf(stderr, "%s %s: %s\n", __FILENAME__, __PRETTY_FUNCTION__, buffer); \
}

#define DEBUG_ADV(OPS) std::cerr << __FILENAME__ << " " << __PRETTY_FUNCTION__ << OPS << std::endl

// usage example: DEBUG_ADV("hello world " << (some python value) << " <- is a python value!");

#else 

#define DEBUG(...)
#define DEBUG_ADV(OPS)

#endif

#ifdef JOHN_DEBUG_ON

#include <cstring>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define J_DEBUG(message, ...) { \
    char buffer[1 << 10]; \
    sprintf(buffer, message, ##__VA_ARGS__); \
    fprintf(stderr, "%s %s: %s\n", __FILENAME__, __PRETTY_FUNCTION__, buffer); \
}

#else 

#define J_DEBUG(...)

#endif

#endif