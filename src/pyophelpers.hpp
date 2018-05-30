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
#include <variant>
#include "../lib/debug.hpp"
#include "../src/builtins/builtins.hpp"

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

struct mult_visitor: public numeric_visitor<op_mult> {
    /*
        the add visitor is simply a numeric_visitor with op_add but also
        including one additional function for string addition
    */
    using numeric_visitor<op_mult>::operator();

    mult_visitor(FrameState &frame) : numeric_visitor<op_mult>(frame) { };
    
    void operator()(ValueList v1, int64_t v2) const {
        ValueList newList = alloc.heap_list.make();
        auto& values = newList->values;
        for (size_t i = 0; i < v2; ++i) {
            for (auto& val : *v1) {
                values.push_back(val);
            }
        }
        frame.value_stack.push_back(newList);
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

    Value operator()(ValueList& list, ValuePyObject& object) {
        if (object->static_attrs == builtins::slice_class) {
            Value start = object->get_attr("start");
            Value stop = object->get_attr("stop");
            Value step = object->get_attr("step");

            int64_t i_start;
            if (auto pstart = std::get_if<int64_t>(&start)) {
                i_start = *pstart;
            } else 
                i_start = 0;

            int64_t i_stop;
            if (auto pstop = std::get_if<int64_t>(&stop)) {
                i_stop = *pstop;
            } else 
                i_stop = list->size();

            int64_t i_step;
            if (auto pstep = std::get_if<int64_t>(&step)) {
                i_step = *pstep;
            } else 
                i_step = 1;

            size_t sstart = i_start >= 0 ? i_start : list->size() + i_start;
            size_t sstop = i_stop >= 0 ? i_stop : list->size() + i_stop;

            ValueList newList = alloc.heap_list.make();
            while (sstart < sstop) {
                newList->values.push_back(list->values[sstart]);
                sstart += i_step;
            }
        } else {
            throw pyerror("List can only be indexed with subclasses of 'Slice'");
        }
    }

    template<typename A, typename B>
    Value operator()(A a , B b) {
        std::stringstream ss;
        ss << "attempted to subscript " << a << "[" << b << "] - types not valid for subscript operator";

        throw pyerror(ss.str());
    }
};

struct store_subscr_visitor {
    Value value;

    void operator()(ValueList list, int64_t k) const {
        if (k < 0 || k > list->values.size()) {
            throw pyerror("list index out of range");
        }
        list->values[k] = std::move(value);
    }
    
    void operator()(ValueList& list, ValuePyObject object) const {
        try {
            if (object->static_attrs == builtins::slice_class) {
                DEBUG_ADV("store subscript detected that the argument is a list slice");
                ValueList listValue = std::get<ValueList>(value);
                Value start = object->get_attr("start");
                Value stop = object->get_attr("stop");
                Value step = object->get_attr("step");

                int64_t i_start;
                if (auto pstart = std::get_if<int64_t>(&start)) {
                    i_start = *pstart;
                } else 
                    i_start = 0;

                int64_t i_stop;
                if (auto pstop = std::get_if<int64_t>(&stop)) {
                    i_stop = *pstop;
                } else 
                    i_stop = list->size();

                int64_t i_step;
                if (auto pstep = std::get_if<int64_t>(&step)) {
                    i_step = *pstep;
                } else 
                    i_step = 1;

                size_t sstart = i_start >= 0 ? i_start : list->size() + i_start;
                size_t sstop = i_stop >= 0 ? i_stop : list->size() + i_stop;
                DEBUG_ADV("detected start: " << sstart << " stop: " << sstop << " step size: " << i_step);

                size_t index = 0;
                while (sstart < sstop) {
                    if (index >= listValue->size()) {
                        throw pyerror("index out of range, no more values to assign in slice assignment");
                    }
                    list->values[sstart] = listValue->values[index++];
                    sstart += i_step;
                }
            } else {
                throw pyerror("List can only be indexed with subclasses of 'Slice'");
            }
        } catch (std::bad_variant_access& e) {
            throw pyerror("Must be assigning a list of values");
        }
        
    }

    void operator()(auto value, auto key) const {
        DEBUG_ADV("enocuntered an error");
        std::stringstream ss;
        ss << "store[] is not a valid operation on " << Value(value) << std::endl;
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

struct unpack_sequence_visitor {
    FrameState& frame;
    uint64_t arg;
    
    void operator()(ValueList list) {
        if (list->size() < arg) {
            std::stringstream ss;
            ss << "not enough values to unpack, expected: " << arg << " got: " << list->size();
            throw pyerror(ss.str());
        }
        for (size_t i = 0; i < arg; ++i) {
            frame.value_stack.push_back(list->values[i]);
        }
    }

    void operator()(ValueTuple list) {
        if (list->size() < arg) {
            std::stringstream ss;
            ss << "not enough values to unpack, expected: " << arg << " got: " << list->size();
            throw pyerror(ss.str());
        }
        for (int64_t i = arg - 1; i > 0; --i) {
            frame.value_stack.push_back(list->values[i]);
        }
    }

    void operator()(auto value) {
        throw pyerror("unpack_sequence_visitor not implemented for type");
    }
};


}

}

#endif