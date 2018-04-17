#ifndef OPTFLAGS_H
#define OPTFLAGS_H

/*
    whenever an optimization is implemented it should be feature flagged 
    by a macro pound defined in this file 
*/

#define OPT_FRAME_NS_LOCAL_SHORTCUT

// #define DEBUG_ON
#define DEBUG_LEVEL 2 // 1 is light debug printing, 2 includes stack dumps and more expensive debug

#endif