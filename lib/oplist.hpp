#ifndef OPLIST_H
#define OPLIST_H
#include <stdint.h>
namespace py {
namespace op {
extern const char *name[256];

const uint8_t HAVE_ARGUMENT = 0x5a;
const uint8_t SPECIAL_EXTENDED_ARG = 0x90;
inline uint8_t operand_length(uint8_t opcode) {
    if (opcode < HAVE_ARGUMENT) return 0;
    return 1;
}
const uint8_t POP_TOP         = 0x01;
const uint8_t ROT_TWO         = 0x02;
const uint8_t ROT_THREE       = 0x03;
const uint8_t DUP_TOP         = 0x04;
const uint8_t DUP_TOP_TWO     = 0x05;
const uint8_t NOP             = 0x09;
const uint8_t UNARY_POSITIVE  = 0x0a;
const uint8_t UNARY_NEGATIVE  = 0x0b;
const uint8_t UNARY_NOT       = 0x0c;
const uint8_t UNARY_INVERT    = 0x0f;
const uint8_t BINARY_MATRIX_MULTIPLY = 0x10;
const uint8_t INPLACE_MATRIX_MULTIPLY = 0x11;
const uint8_t BINARY_POWER    = 0x13;
const uint8_t BINARY_MULTIPLY = 0x14;
const uint8_t BINARY_MODULO   = 0x16;
const uint8_t BINARY_ADD      = 0x17;
const uint8_t BINARY_SUBTRACT = 0x18;
const uint8_t BINARY_SUBSCR   = 0x19;
const uint8_t BINARY_FLOOR_DIVIDE = 0x1a;
const uint8_t BINARY_TRUE_DIVIDE = 0x1b;
const uint8_t INPLACE_FLOOR_DIVIDE = 0x1c;
const uint8_t INPLACE_TRUE_DIVIDE = 0x1d;
const uint8_t GET_AITER       = 0x32;
const uint8_t GET_ANEXT       = 0x33;
const uint8_t BEFORE_ASYNC_WITH = 0x34;
const uint8_t INPLACE_ADD     = 0x37;
const uint8_t INPLACE_SUBTRACT = 0x38;
const uint8_t INPLACE_MULTIPLY = 0x39;
const uint8_t INPLACE_MODULO  = 0x3b;
const uint8_t STORE_SUBSCR    = 0x3c;
const uint8_t DELETE_SUBSCR   = 0x3d;
const uint8_t BINARY_LSHIFT   = 0x3e;
const uint8_t BINARY_RSHIFT   = 0x3f;
const uint8_t BINARY_AND      = 0x40;
const uint8_t BINARY_XOR      = 0x41;
const uint8_t BINARY_OR       = 0x42;
const uint8_t INPLACE_POWER   = 0x43;
const uint8_t GET_ITER        = 0x44;
const uint8_t GET_YIELD_FROM_ITER = 0x45;
const uint8_t PRINT_EXPR      = 0x46;
const uint8_t LOAD_BUILD_CLASS = 0x47;
const uint8_t YIELD_FROM      = 0x48;
const uint8_t GET_AWAITABLE   = 0x49;
const uint8_t INPLACE_LSHIFT  = 0x4b;
const uint8_t INPLACE_RSHIFT  = 0x4c;
const uint8_t INPLACE_AND     = 0x4d;
const uint8_t INPLACE_XOR     = 0x4e;
const uint8_t INPLACE_OR      = 0x4f;
const uint8_t BREAK_LOOP      = 0x50;
const uint8_t WITH_CLEANUP_START = 0x51;
const uint8_t WITH_CLEANUP_FINISH = 0x52;
const uint8_t RETURN_VALUE    = 0x53;
const uint8_t IMPORT_STAR     = 0x54;
const uint8_t SETUP_ANNOTATIONS = 0x55;
const uint8_t YIELD_VALUE     = 0x56;
const uint8_t POP_BLOCK       = 0x57;
const uint8_t END_FINALLY     = 0x58;
const uint8_t POP_EXCEPT      = 0x59;
const uint8_t STORE_NAME      = 0x5a;
const uint8_t DELETE_NAME     = 0x5b;
const uint8_t UNPACK_SEQUENCE = 0x5c;
const uint8_t FOR_ITER        = 0x5d;
const uint8_t UNPACK_EX       = 0x5e;
const uint8_t STORE_ATTR      = 0x5f;
const uint8_t DELETE_ATTR     = 0x60;
const uint8_t STORE_GLOBAL    = 0x61;
const uint8_t DELETE_GLOBAL   = 0x62;
const uint8_t LOAD_CONST      = 0x64;
const uint8_t LOAD_NAME       = 0x65;
const uint8_t BUILD_TUPLE     = 0x66;
const uint8_t BUILD_LIST      = 0x67;
const uint8_t BUILD_SET       = 0x68;
const uint8_t BUILD_MAP       = 0x69;
const uint8_t LOAD_ATTR       = 0x6a;
const uint8_t COMPARE_OP      = 0x6b;
const uint8_t IMPORT_NAME     = 0x6c;
const uint8_t IMPORT_FROM     = 0x6d;
const uint8_t JUMP_FORWARD    = 0x6e;
const uint8_t JUMP_IF_FALSE_OR_POP = 0x6f;
const uint8_t JUMP_IF_TRUE_OR_POP = 0x70;
const uint8_t JUMP_ABSOLUTE   = 0x71;
const uint8_t POP_JUMP_IF_FALSE = 0x72;
const uint8_t POP_JUMP_IF_TRUE = 0x73;
const uint8_t LOAD_GLOBAL     = 0x74;
const uint8_t CONTINUE_LOOP   = 0x77;
const uint8_t SETUP_LOOP      = 0x78;
const uint8_t SETUP_EXCEPT    = 0x79;
const uint8_t SETUP_FINALLY   = 0x7a;
const uint8_t LOAD_FAST       = 0x7c;
const uint8_t STORE_FAST      = 0x7d;
const uint8_t DELETE_FAST     = 0x7e;
const uint8_t STORE_ANNOTATION = 0x7f;
const uint8_t RAISE_VARARGS   = 0x82;
const uint8_t CALL_FUNCTION   = 0x83;
const uint8_t MAKE_FUNCTION   = 0x84;
const uint8_t BUILD_SLICE     = 0x85;
const uint8_t MAKE_CLOSURE    = 0x86;
const uint8_t LOAD_CLOSURE    = 0x87;
const uint8_t LOAD_DEREF      = 0x88;
const uint8_t STORE_DEREF     = 0x89;
const uint8_t DELETE_DEREF    = 0x8a;
const uint8_t CALL_FUNCTION_KW = 0x8d;
const uint8_t CALL_FUNCTION_EX = 0x8e;
const uint8_t SETUP_WITH      = 0x8f;
const uint8_t EXTENDED_ARG    = 0x90;
const uint8_t LIST_APPEND     = 0x91;
const uint8_t SET_ADD         = 0x92;
const uint8_t MAP_ADD         = 0x93;
const uint8_t LOAD_CLASSDEREF = 0x94;
const uint8_t BUILD_LIST_UNPACK = 0x95;
const uint8_t BUILD_MAP_UNPACK = 0x96;
const uint8_t BUILD_MAP_UNPACK_WITH_CALL = 0x97;
const uint8_t BUILD_TUPLE_UNPACK = 0x98;
const uint8_t BUILD_SET_UNPACK = 0x99;
const uint8_t SETUP_ASYNC_WITH = 0x9a;
const uint8_t FORMAT_VALUE    = 0x9b;
const uint8_t BUILD_CONST_KEY_MAP = 0x9c;
const uint8_t BUILD_STRING    = 0x9d;
const uint8_t BUILD_TUPLE_UNPACK_WITH_CALL = 0x9e;
namespace cmp {

const uint8_t LT = 0;
const uint8_t LTE = 1;
const uint8_t EQ = 2;
const uint8_t NEQ = 3;
const uint8_t GT = 4;
const uint8_t GTE = 5;
const uint8_t IN = 6;
const uint8_t NOTIN = 7;
const uint8_t IS = 8;
const uint8_t ISNOT = 9;
const uint8_t EXCEPTION_MATCH = 10;

extern const char* name[];

}
}
}
#endif
