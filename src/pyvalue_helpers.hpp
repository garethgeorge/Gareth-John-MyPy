#pragma once

#ifndef VALUE_HELPERS_H
#define VALUE_HELPERS_H

#include <string>
#include "pyvalue.hpp"
#include <tuple>
#include "pyframe.hpp"
#include "pyinterpreter.hpp"
#include <variant>

using std::string;

namespace py {
namespace value_helper {

// helper function for creating a class with overloads
// see http://en.cppreference.com/w/cpp/utility/variant/visit 
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// a visitor that, given a type, returns its truthyness 
struct visitor_is_truthy {
    bool operator()(double) const;
    bool operator()(int64_t) const;
    bool operator()(const ValueString&) const;
    bool operator()(const value::NoneType) const;
    inline bool operator()(bool val) const {
        // inline this to make the common case fast
        return val;
    }
    // TODO: empty sequence, empty mapping
    // https://docs.python.org/2.4/lib/truth.html
    template<typename T>
    bool operator()(T v) const {
        return true;
    }
};

// this visitor evaluates the python repr(obj) method
struct visitor_repr {
    string operator()(bool) const;
    
    string operator()(double) const;

    string operator()(int64_t) const;

    string operator()(const ValueString&) const;

    string operator()(value::NoneType) const;

    string operator()(ValuePyFunction) const;

    string operator()(ValuePyClass) const;

    string operator()(ValuePyObject) const;
    
    template<typename T> 
    string operator()(T) const {
        // TODO: use typetraits to generate this.
        throw pyerror("unimplemented repr for type");
    }
};

// this visitor evaluates the python str(obj) method, only differs from
// repr in the case of true objects so as you can see it just wraps repr
struct visitor_str : public visitor_repr {
    using visitor_repr::operator();
    // TODO: add specialization for handling additional types
};

struct call_visitor {
    /*
        the call visitor is a helpful visitor class that actually includes 
        some amount of state, it takes the argument list as well as the current
        frame. 

        It may be possible to refactor this into a visitor using the lambda
        style syntax ideally.
    */

    FrameState& frame;
    std::vector<Value>& args;
    call_visitor(FrameState& frame, std::vector<Value>& args) : frame(frame), args(args) {}

    void operator()(const ValueCFunction& func) const {
        DEBUG("call_visitor dispatching CFunction->action");
        func->action(frame, args);
    }

    // A PyClass was called like a function, therein creating a PyObject
    void operator()(const ValuePyClass& cls) const {
        DEBUG("Constructing a '%s' Object",std::get<ValueString>(
            (*(cls->attrs))["__qualname__"]
        )->c_str());
        /*for(auto it = cls->attrs.begin();it != cls->attrs.end();++it){
            printf("%s = ",it->first.c_str());
            frame.print_value(it->second);
            printf("\n");
        }*/
        ValuePyObject npo = std::make_shared<value::PyObject>(
            value::PyObject(cls)
        );


        // Check the class if it has an init function
        auto itr = cls->attrs->find("__init__");
        Value vv;
        bool has_init = true;
        if(itr == cls->attrs->end()){
            std::tuple<Value,bool> par_val = value::PyClass::find_attr_in_parents(cls,std::string("__init__"));
            if(std::get<1>(par_val)){
                vv = std::get<0>(par_val);
            } else {
                has_init = false;
            }
        } else {
            vv = itr->second;
        }

        if(has_init){
            // call the init function, pushing the new object as the first argument 'self'
            args.insert(args.begin(),npo);
            std::visit(call_visitor(frame,args),vv);
            
            // Now that a new frame is on the stack, set a flag in it that it's an initializer frame
            frame.interpreter_state->callstack.top().set_class_dynamic_init_flag();
        } 

        // Push the new object on the value stack
        frame.value_stack.push_back(std::move(npo));
    }

    void operator()(const ValuePyFunction& func) const {
        DEBUG("call_visitor dispatching on a PyFunction");

        // Throw an error if too many arguments
        if(args.size() > func->code->co_argcount){
            throw pyerror(std::string("TypeError: " + *(func->name)
                        + " takes " + std::to_string(func->code->co_argcount)
                        + " positional arguments but " + std::to_string(args.size())
                        + " were given"));
        }

        // Push a new FrameState
        frame.interpreter_state->callstack.push(
            std::move( FrameState(frame.interpreter_state, &frame, func->code))
        );
        frame.interpreter_state->callstack.top().initialize_from_pyfunc(func,args);
    }
    
    template<typename T>
    void operator()(T) const {
        throw pyerror(string("can not call object of type ") + typeid(T).name());
    }
};

template<class T>
struct numeric_visitor {
    FrameState& frame;

    numeric_visitor(FrameState& frame) : frame(frame) {}

    void operator()(double v1, double v2) const {
        frame.value_stack.push_back(T::action(v1, v2));
    }
    void operator()(double v1, int64_t v2) const {
        frame.value_stack.push_back(T::action(v1, v2));
    }
    void operator()(int64_t v1, double v2) const {
        frame.value_stack.push_back(T::action(v1, v2));
    } 
    void operator()(int64_t v1, int64_t v2) const {
        frame.value_stack.push_back(T::action(v1, v2));
    }
    
    template<typename OT>
    void operator()(ValuePyObject& v1, OT& v2) const {
        // Get the attribute for it
        std::tuple<Value,bool> res = value::PyObject::find_attr_in_obj(v1,std::string(T::l_attr));
        if(std::get<1>(res)){
            // Call it like a function
            std::vector<Value> args(1, v2);
            std::visit(
                call_visitor(frame, args),
                std::get<0>(res)
            );
        } else {
            throw pyerror(
                string("TypeError: unsupported operand type(s) for ") + T::op_name + string(": '")
                + *(std::get<ValueString>((v1->static_attrs->attrs->at("__qualname__")))) + string("' and '") + string("INT'")
            );
        }
    }
    
    template<typename T1, typename T2>
    void operator()(T1, T2) const {
        throw pyerror(string("type error in numeric_visitor, can not work on values of types ") + typeid(T1).name() + " and " + typeid(T2).name());
    }
};

}
}

#endif
