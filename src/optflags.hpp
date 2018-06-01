#ifndef OPTFLAGS_H
#define OPTFLAGS_H

/*
    whenever an optimization is implemented it should be feature flagged 
    by a macro pound defined in this file 
*/

#define DIRECT_THREADED
#define RECYCLING_ON
#define CHECK_STACK_SIZES

//#define DEBUG_ON
// #define DEBUG_STACK

//#define JOHN_DEBUG_ON

// #define PROFILING_ON

// Output a timestamp at every opcode
// #define PER_OPCODE_PROFILING

// #define GARBAGE_COLLECTION_PROFILING

// #define TYPE_INFORMATION_PROFILING

#endif