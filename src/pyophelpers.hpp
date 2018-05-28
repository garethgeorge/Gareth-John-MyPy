#ifndef PYOPHELPERS_HPP
#define PYOPHELPERS_HPP

#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <variant>
#include <cmath>
#include <cassert>
#include <sstream>
#include <tuple>
#include "../lib/debug.hpp"

#include "pyinterpreter.hpp"
#include "pyvalue_helpers.hpp"

namespace py {

namespace eval_helpers {

using value_helper::numeric_visitor;

/*
    op_ classes are binary operation helper classes,
    when numeric_visitor is templated with a class defining a static method
    T3 action(T1, T2);
    it then applies that method to any numeric input types, and returns the
    resulting value. For objects it should fall back to a lookup into the 
    object's type info table. For any other type it helpfully throws a 
    pyerror indicating a type mismatch was encountered.
*/
struct op_lt { // a < b
    template<typename T1, typename T2>
    static bool action(T1 v1, T2 v2) {
        return v1 < v2;
    }

    constexpr const static char* l_attr = "__lt__";
    constexpr const static char* r_attr = "__gt__";
    constexpr const static char* op_name = "<";
};

struct op_lte { // a <= b
    template<typename T1, typename T2>
    static bool action(T1 v1, T2 v2) {
        return v1 <= v2;
    }

    constexpr const static char* l_attr = "__le__";
    constexpr const static char* r_attr = "__ge__";
    constexpr const static char* op_name = "<=";
};

struct op_gt { // a > b
    template<typename T1, typename T2>
    static bool action(T1 v1, T2 v2) {
        return v1 > v2;
    }

    constexpr const static char* l_attr = "__gt__";
    constexpr const static char* r_attr = "__lt__";
    constexpr const static char* op_name = ">";
};

struct op_gte { // a >= b
    template<typename T1, typename T2>
    static bool action(T1 v1, T2 v2) {
        return v1 >= v2;
    }

    constexpr const static char* l_attr = "__ge__";
    constexpr const static char* r_attr = "__le__";
    constexpr const static char* op_name = ">=";
};

struct op_eq { // a == b
    template<typename T1, typename T2>
    static bool action(T1 v1, T2 v2) {
        return v1 == v2;
    }

    constexpr const static char* l_attr = "__eq__";
    constexpr const static char* r_attr = "__eq__";
    constexpr const static char* op_name = "==";
};

struct op_neq { // a != b
    template<typename T1, typename T2>
    static bool action(T1 v1, T2 v2) {
        return v1 != v2;
    }

    constexpr const static char* l_attr = "__ne__";
    constexpr const static char* r_attr = "__ne__";
    constexpr const static char* op_name = "!=";
};

struct op_sub { // a - b
    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        return v1 - v2;
    }

    constexpr const static char* l_attr = "__sub__";
    constexpr const static char* r_attr = "__rsub__";
    constexpr const static char* op_name = "-";
};

struct op_add { // a + b
    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        return v1 + v2;
    }

    constexpr const static char* l_attr = "__add__";
    constexpr const static char* r_attr = "__radd__";
    constexpr const static char* op_name = "+";
};

struct op_mult { // a * b
    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        return v1 * v2;
    }

    constexpr const static char* l_attr = "__mul__";
    constexpr const static char* r_attr = "__rmul__";
    constexpr const static char* op_name = "*";
};

struct op_divide { // a // b
    template<typename T1, typename T2>        
    static auto action(T1 v1, T2 v2) {
        return (int64_t)(v1 / v2);
    }

    constexpr const static char* l_attr = "__floordiv__";
    constexpr const static char* r_attr = "__rfloordiv__";
    constexpr const static char* op_name = "//";
};

struct op_true_div { // a / b
    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        return (double)v1 / (double)v2;
    }

    constexpr const static char* l_attr = "__truediv__";
    constexpr const static char* r_attr = "__rtruediv__";
    constexpr const static char* op_name = "/";
};

struct op_pow { // a ** b
    static auto action(int64_t v1,int64_t v2) {
        return (int64_t)pow(v1,v2);
    }

    static auto action(double v1,int64_t v2) {
        return pow(v1,v2);
    }

    static auto action(int64_t v1,double v2) {
        return pow(v1,v2);
    }

    static auto action(double v1,double v2) {
        return pow(v1,v2);
    }

    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        throw pyerror("Invalid operands for **\n");
        return value::NoneType();
    }

    constexpr const static char* l_attr = "__pow__";
    constexpr const static char* r_attr = "__rpow__";
    constexpr const static char* op_name = "**";
};

struct op_lshift { // a << b
    static auto action(int64_t v1,int64_t v2) {
        return v1 << v2;
    }

    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        throw pyerror("Invalid operands for <<\n");
        return value::NoneType();
    }

    constexpr const static char* l_attr = "__lshift__";
    constexpr const static char* r_attr = "__rlshift__";
    constexpr const static char* op_name = "<<";
};

struct op_rshift { // a >> b
    static auto action(int64_t v1,int64_t v2) {
        return v1 >> v2;
    }

    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        throw pyerror("Invalid operands for >>\n");
        return value::NoneType();
    }

    constexpr const static char* l_attr = "__rshift__";
    constexpr const static char* r_attr = "__rrshift__";
    constexpr const static char* op_name = ">>";
};

struct op_and { // a & b
    static auto action(int64_t v1,int64_t v2) {
        return v1 & v2;
    }

    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        throw pyerror("Invalid operands for &\n");
        return value::NoneType();
    }

    constexpr const static char* l_attr = "__and__";
    constexpr const static char* r_attr = "__rand__";
    constexpr const static char* op_name = "&";
};

struct op_or { // a | b
    static auto action(int64_t v1,int64_t v2) {
        return v1 | v2;
    }

    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        throw pyerror("Invalid operands for |\n");
        return value::NoneType();
    }

    constexpr const static char* l_attr = "__or__";
    constexpr const static char* r_attr = "__ror__";
    constexpr const static char* op_name = "|";
};

struct op_xor { // a ^ b
    static auto action(int64_t v1,int64_t v2) {
        return v1 ^ v2;
    }

    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        throw pyerror("Invalid operands for ^\n");
        return value::NoneType();
    }

    constexpr const static char* l_attr = "__xor__";
    constexpr const static char* r_attr = "__rxor__";
    constexpr const static char* op_name = "^";
};

struct op_modulo { // a % b

    // for integer values it is sufficient to use the default % operator
    static auto action(int64_t v1, int64_t v2) {
        return v1 % v2;
    }

    // for other type we fall back to the fmod operator, a more general
    // mod which always returns a float value of sufficient precision
    template<typename T1, typename T2>
    static auto action(T1 v1, T2 v2) {
        return std::fmod(v1, v2);
    }

    constexpr const static char* l_attr = "__mod__";
    constexpr const static char* r_attr = "__rmod__";
    constexpr const static char* op_name = "%";
};

struct add_visitor: public numeric_visitor<op_add> {
    /*
        the add visitor is simply a numeric_visitor with op_add but also
        including one additional function for string addition
    */
    using numeric_visitor<op_add>::operator();

    add_visitor(FrameState &frame) : numeric_visitor<op_add>(frame) { };
    
    void operator()(const gc_ptr<const std::string>& v1, const gc_ptr<const std::string> &v2) const {
        frame.value_stack.push_back(
            alloc.heap_string.make(*v1 + *v2)
        );
    }
};

struct binary_subscr_visitor {
    /*
        the binary subscript visitor is used to implement
        Implements TOS = TOS1[TOS].
    */
    Value operator()(ValueList& list, int64_t index) {
        auto& values = list->values;
        if (index < 0) {
            index = values.size() + index;
        }
        if (index >= values.size()) {
            std::stringstream ss;
            ss << "attempted list access out of range LIST[" << index << "] but list only held " << values.size() << " elements.";
            throw pyerror(ss.str());
        } 

        return values[index];
    }

    template<typename A, typename B>
    Value operator()(A a , B b) {
        std::stringstream ss;
        ss << "attempted to subscript " << a << "[" << b << "] - types not valid for subscript operator";

        throw pyerror(ss.str());
    }
};

// Used in initialize_from_pyfunc to set the first argument of 
struct set_implicit_arg_visitor {
    FrameState& frame;
    
    set_implicit_arg_visitor(FrameState& frame) : frame(frame) {}

    void operator()(const ValuePyClass& cls) const {
        if(cls){
            frame.add_to_ns_local(
                frame.code->co_varnames[0],
                cls
            );
        }
    }

    void operator()(const ValuePyObject& obj) const {
        if(obj){
            frame.add_to_ns_local(
                frame.code->co_varnames[0],
                obj
            );
        }
    }
    
    template<typename T>
    void operator()(T) const {
        throw pyerror("can not set implicit arg on this type ");
    }
};

struct store_subscr_visitor {
    Value& key;
    Value& value;

    void operator()(ValueList& list) {
        try {
            int64_t k = std::get<int64_t>(key);
            if (k < 0 || k > list->values.size()) {
                throw pyerror("list index out of range");
            }
            list->values[k] = std::move(value);
        } catch (std::bad_variant_access& err) {
            throw pyerror("list key must be integer index");
        }

    }

    template<typename T>
    void operator()(T value) const {
        std::stringstream ss;
        ss << "store[] is not a valid operation on " << Value(value) << std::endl;
        throw pyerror(ss.str());
    }
};


}

}

#endif