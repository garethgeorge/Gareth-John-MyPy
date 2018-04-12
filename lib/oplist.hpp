#ifndef OPLIST_H

#include <stdint.h>

namespace mypy {
extern const char *oplist[];
extern uint8_t opargs[];

namespace op {

const uint8_t POP_TOP = 1;
const uint8_t ROT_TWO = 2;
const uint8_t ROT_THREE = 3;
const uint8_t DUP_TOP = 4;
const uint8_t DUP_TOP_TWO = 5;
const uint8_t NOP = 9;
const uint8_t UNARY_POSITIVE = 10;
const uint8_t UNARY_NEGATIVE = 11;
const uint8_t UNARY_NOT = 12;
const uint8_t UNARY_INVERT = 15;
const uint8_t BINARY_MATRIX_MULTIPLY = 16;
const uint8_t INPLACE_MATRIX_MULTIPLY = 17;
const uint8_t BINARY_POWER = 19;
const uint8_t BINARY_MULTIPLY = 20;
const uint8_t BINARY_MODULO = 22;
const uint8_t BINARY_ADD = 23;
const uint8_t BINARY_SUBTRACT = 24;
const uint8_t BINARY_SUBSCR = 25;
const uint8_t BINARY_FLOOR_DIVIDE = 26;
const uint8_t BINARY_TRUE_DIVIDE = 27;
const uint8_t INPLACE_FLOOR_DIVIDE = 28;
const uint8_t INPLACE_TRUE_DIVIDE = 29;
const uint8_t GET_AITER = 50;
const uint8_t GET_ANEXT = 51;
const uint8_t BEFORE_ASYNC_WITH = 52;
const uint8_t INPLACE_ADD = 55;
const uint8_t INPLACE_SUBTRACT = 56;
const uint8_t INPLACE_MULTIPLY = 57;
const uint8_t INPLACE_MODULO = 59;
const uint8_t STORE_SUBSCR = 60;
const uint8_t DELETE_SUBSCR = 61;
const uint8_t BINARY_LSHIFT = 62;
const uint8_t BINARY_RSHIFT = 63;
const uint8_t BINARY_AND = 64;
const uint8_t BINARY_XOR = 65;
const uint8_t BINARY_OR = 66;
const uint8_t INPLACE_POWER = 67;
const uint8_t GET_ITER = 68;
const uint8_t GET_YIELD_FROM_ITER = 69;
const uint8_t PRINT_EXPR = 70;
const uint8_t LOAD_BUILD_CLASS = 71;
const uint8_t YIELD_FROM = 72;
const uint8_t GET_AWAITABLE = 73;
const uint8_t INPLACE_LSHIFT = 75;
const uint8_t INPLACE_RSHIFT = 76;
const uint8_t INPLACE_AND = 77;
const uint8_t INPLACE_XOR = 78;
const uint8_t INPLACE_OR = 79;
const uint8_t BREAK_LOOP = 80;
const uint8_t WITH_CLEANUP_START = 81;
const uint8_t WITH_CLEANUP_FINISH = 82;
const uint8_t RETURN_VALUE = 83;
const uint8_t IMPORT_STAR = 84;
const uint8_t SETUP_ANNOTATIONS = 85;
const uint8_t YIELD_VALUE = 86;
const uint8_t POP_BLOCK = 87;
const uint8_t END_FINALLY = 88;
const uint8_t POP_EXCEPT = 89;
const uint8_t STORE_NAME = 90;
const uint8_t DELETE_NAME = 91;
const uint8_t UNPACK_SEQUENCE = 92;
const uint8_t FOR_ITER = 93;
const uint8_t UNPACK_EX = 94;
const uint8_t STORE_ATTR = 95;
const uint8_t DELETE_ATTR = 96;
const uint8_t STORE_GLOBAL = 97;
const uint8_t DELETE_GLOBAL = 98;
const uint8_t LOAD_CONST = 100;
const uint8_t LOAD_NAME = 101;
const uint8_t BUILD_TUPLE = 102;
const uint8_t BUILD_LIST = 103;
const uint8_t BUILD_SET = 104;
const uint8_t BUILD_MAP = 105;
const uint8_t LOAD_ATTR = 106;
const uint8_t COMPARE_OP = 107;
const uint8_t IMPORT_NAME = 108;
const uint8_t IMPORT_FROM = 109;
const uint8_t JUMP_FORWARD = 110;
const uint8_t JUMP_IF_FALSE_OR_POP = 111;
const uint8_t JUMP_IF_TRUE_OR_POP = 112;
const uint8_t JUMP_ABSOLUTE = 113;
const uint8_t POP_JUMP_IF_FALSE = 114;
const uint8_t POP_JUMP_IF_TRUE = 115;
const uint8_t LOAD_GLOBAL = 116;
const uint8_t CONTINUE_LOOP = 119;
const uint8_t SETUP_LOOP = 120;
const uint8_t SETUP_EXCEPT = 121;
const uint8_t SETUP_FINALLY = 122;
const uint8_t LOAD_FAST = 124;
const uint8_t STORE_FAST = 125;
const uint8_t DELETE_FAST = 126;
const uint8_t STORE_ANNOTATION = 127;
const uint8_t RAISE_VARARGS = 130;
const uint8_t CALL_FUNCTION = 131;
const uint8_t MAKE_FUNCTION = 132;
const uint8_t BUILD_SLICE = 133;
const uint8_t LOAD_CLOSURE = 135;
const uint8_t LOAD_DEREF = 136;
const uint8_t STORE_DEREF = 137;
const uint8_t DELETE_DEREF = 138;
const uint8_t CALL_FUNCTION_KW = 141;
const uint8_t CALL_FUNCTION_EX = 142;
const uint8_t SETUP_WITH = 143;
const uint8_t EXTENDED_ARG = 144;
const uint8_t LIST_APPEND = 145;
const uint8_t SET_ADD = 146;
const uint8_t MAP_ADD = 147;
const uint8_t LOAD_CLASSDEREF = 148;
const uint8_t BUILD_LIST_UNPACK = 149;
const uint8_t BUILD_MAP_UNPACK = 150;
const uint8_t BUILD_MAP_UNPACK_WITH_CALL = 151;
const uint8_t BUILD_TUPLE_UNPACK = 152;
const uint8_t BUILD_SET_UNPACK = 153;
const uint8_t SETUP_ASYNC_WITH = 154;
const uint8_t FORMAT_VALUE = 155;
const uint8_t BUILD_CONST_KEY_MAP = 156;
const uint8_t BUILD_STRING = 157;
const uint8_t BUILD_TUPLE_UNPACK_WITH_CALL = 158;
}

}


#endif