#include <stdio.h>
#include <iostream>
#include <utility>
#include <algorithm>
#include <variant>
#include <cmath>
#include <cassert>
#include <sstream>
#include <tuple>
#include "pyvalue_helpers.hpp"
#include "pyframe.hpp"
#include "pyinterpreter.hpp"
#include "../lib/oplist.hpp"
// #define DEBUG_ON
#include "../lib/debug.hpp"

using std::string;

namespace py {

FrameState::FrameState(const ValueCode& code) 
{
    DEBUG("constructed a new frame");
    this->ns_local = std::make_shared<std::unordered_map<std::string, Value>>();
    this->code = code;
    DEBUG("reserved %lu bytes for the stack", code->co_stacksize);
    this->value_stack.reserve(code->co_stacksize);
}

// Construct a framestate meant to initialize everything static about a class
FrameState::FrameState(const ValueCode& code, ValuePyClass& init_class)
{
    DEBUG("constructed a new frame for statically initializing a class");
    // Everything is mostly the same, but our local namespace is also the class's
    this->code = code;
    DEBUG("reserved %lu bytes for the stack", code->co_stacksize);
    this->value_stack.reserve(code->co_stacksize);
    this->init_class = init_class;
    this->ns_local = this->init_class->attrs;
    this->set_flag(FrameState::FLAG_CLASS_STATIC_INIT);
}

// Find an attribute in the parents of a class
std::tuple<Value,bool> value::PyClass::find_attr_in_parents(
                                    const ValuePyClass& cls,
                                    const std::string& attr
) {
    DEBUG_ADV("Searching parents of class '" 
        << (*(std::get<ValueString>( (*(cls->attrs))["__qualname__"]))).c_str()
        << "' for attr '" << attr);
    // Method Resolution Order already stored in the order parents are stored in
    for(int i = 0;i < cls->parents.size();i++){
        DEBUG("Checking parent %s\n", 
            // Here there be dragons
            std::get<ValueString>((cls->parents[i]->attrs->at("__qualname__")))->c_str()
        );
        auto itr = cls->parents[i]->attrs->find(attr);
        if(itr != cls->parents[i]->attrs->end()){
            return std::tuple<Value,bool>(itr->second,true);
        }
    }
    return std::tuple<Value,bool>(value::NoneType(),false);
}

std::tuple<Value,bool> value::PyObject::find_attr_in_obj(
                                            const ValuePyObject& obj,
                                            const std::string& attr
){
    auto itr = obj->attrs->find(attr);
    if(itr != obj->attrs->end()){
        return std::tuple<Value,bool>(itr->second,true);
    } else {
        // Default to statics if not found
        auto itr_2 = obj->static_attrs->attrs->find(attr);
        Value static_val;
        if(itr_2 == obj->static_attrs->attrs->end()){
            std::tuple<Value,bool> par_val = value::PyClass::find_attr_in_parents(obj->static_attrs,attr);
            if(std::get<1>(par_val)){
                static_val = std::get<0>(par_val);
            } else {
                // Found in no parents. return nothing
                return std::tuple<Value,bool>(value::NoneType(),false);
            }
        } else {
            static_val = itr_2->second;
        }
        
        // Check to see if it is a PyFunc, and if so make it's self to obj
        // This essentially accomplishes lazy initialization of instance functions
        auto pf = std::get_if<ValuePyFunction>(&static_val);
        if(pf != NULL){
            // Push a new PyFunc with self set to obj or obj's class
            // Store it so that next time it is accessed it will be found in attrs
            //if(((*pf)->flags & value::CLASS_METHOD)
            //|| ((*pf)->flags & value::STATIC_METHOD) ){
            if((*pf)->flags & (value::CLASS_METHOD | value::STATIC_METHOD)){
                // All good, just return
                return std::tuple<Value,bool>((*pf),true);
            } else {
                DEBUG("Instantiating instance method");
                // Create an instance method
                Value npf = std::make_shared<value::PyFunc>(
                    value::PyFunc {
                        (*pf)->name,
                        (*pf)->code,
                        (*pf)->def_args,
                        obj, // Instance method's self
                        value::INSTANCE_METHOD}
                );
                obj->store_attr(attr,npf); // Does this create a shared_ptr cycle
                return std::tuple<Value,bool>(npf,true);
            }
        } else {
            // All is well, push as normal
            return std::tuple<Value,bool>(static_val,true);
        }
    }
}

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

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
    };

    struct op_lte { // a <= b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 <= v2;
        }

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
    };

    struct op_gt { // a > b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 > v2;
        }

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
    };

    struct op_gte { // a >= b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 >= v2;
        }

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
    };

    struct op_eq { // a == b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 == v2;
        }

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
    };

    struct op_neq { // a != b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 != v2;
        }

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
    };

    struct op_sub { // a - b
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 - v2;
        }

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
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

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
    };

    struct op_divide { // a / b
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 / v2;
        }

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
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

        constexpr const static char* l_attr = "__add__";
        constexpr const static char* r_attr = "__radd__";
        constexpr const static char* op_name = "+";
    };
    
    struct add_visitor: public numeric_visitor<op_add> {
        /*
            the add visitor is simply a numeric_visitor with op_add but also
            including one additional function for string addition
        */
        using numeric_visitor<op_add>::operator();

        add_visitor(FrameState &frame) : numeric_visitor<op_add>(frame) { };
        
        void operator()(const std::shared_ptr<std::string>& v1, const std::shared_ptr<std::string> &v2) const {
            frame.value_stack.push_back(std::make_shared<std::string>(*v1 + *v2));
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

    struct for_iter_visitor {
        FrameState &frame;
        uint64_t arg;

        inline void operator ()(value::PyGenerator& gen) {
            frame.check_stack_size(1);
            DEBUG_ADV("\tENCOUNTERED op::FOR_ITER, no values on stack so we must yield control to get the values");
            // pushes the generator frame as the current frame,
            // and sets its parent_frame to be this frame since we are 
            // the cur_frame right now
            gen.frame->parent_frame = frame.interpreter_state->cur_frame;
            frame.interpreter_state->push_frame(gen.frame);
        }

        inline void operator ()(bool haveValue) {
            frame.check_stack_size(1);
            if (haveValue) {
                DEBUG_ADV("\tVALUES ON THE STACK, POPPING THOSE VALUES. HAPPY HAPPY.");
                frame.value_stack.pop_back();
                frame.r_pc++;
            } else {
                DEBUG_ADV("\tITERATOR EXHAUSTED, JUMPING TO END OF LOOP");
                frame.r_pc = frame.code->pc_map[frame.code->instructions[frame.r_pc].bytecode_index + arg] + 1;
            }
    
        }

        template<typename T>
        void operator () (T& ) {
            throw pyerror("FOR_ITER expects iterable");
        }
    };

}

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
        // Do nothing
        return;
    }
};

void FrameState::initialize_from_pyfunc(const ValuePyFunction& func, std::vector<Value>& args){
    // Calculate which argument is the first argument with a default value
    // Also whether or not the very first argument is self (or class)
    // This could be stored in PyFunc struct but that is a tiny space tradeoff vs tiny time tradeoff
    int first_def_arg = this->code->co_argcount - func->def_args->size();
   
    // Set the implicit argument
    bool has_implicit_arg = func->flags & (value::CLASS_METHOD | value::INSTANCE_METHOD);
    if(has_implicit_arg) std::visit(set_implicit_arg_visitor(*this),func->self);

    int first_arg_is_self = (has_implicit_arg ? 1 : 0);

    J_DEBUG("(Assigning the following values to names:\n");

    // put values into the local pool
    // the name is the constant (co_varnames) at the argument number it is
    // the value has been passed in or uses the default
    // If we are an instance method, skip the first arg as it was set above
    for(int i = (has_implicit_arg ? 1 : 0); i < this->code->co_argcount; i++){
        // The arg to consider may not quite align with i
        int arg_num = i - first_arg_is_self;

        //Error if not given enough arguments
        //TypeError: simplefunc() missing 2 required positional arguments: 'a' and 'd'
        if(arg_num < first_def_arg && arg_num >= args.size()){
            int missing_num = first_def_arg - arg_num;

            std::stringstream ss;
            ss << "TypeError: " << Value(func) << " missing " << (missing_num) << " required positional arguments:";
            for (; arg_num < first_def_arg; arg_num++) {
                ss << "'" << this->code->co_varnames[i] << "'";
                if (arg_num < first_def_arg - 1) ss << ",";
            }

            // List the missing params
            throw pyerror(ss.str());
            return;
        }

        J_DEBUG("Name: %s\n",this->code->co_varnames[i].c_str());
        J_DEBUG("Value: ");
        #ifdef JOHN_DEBUG_ON
        print_value(arg_num < args.size() ? args[arg_num] : (*(func->def_args))[arg_num - first_def_arg]);
        #endif

        // The argument exists, save it
        add_to_ns_local(
            // Read the name to save to from the constants pool
            this->code->co_varnames[i], 
            // read the value from passed in args, or else the default
            arg_num < args.size() ? std::move(args[arg_num]) : (*(func->def_args))[arg_num - first_def_arg] 
        );
    }
}

// Add a value to the ns local
void FrameState::add_to_ns_local(const std::string& name,Value&& v){
    this->ns_local->emplace(name,v);
}

void FrameState::print_value(Value& val) {
    std::visit(value_helper::overloaded {
            [](auto&& arg) { 
                throw pyerror(string("unimplemented stack printer for stack value: ") + typeid(arg).name());
            },
            [](double arg) { std::cerr << "double(" << arg << ")"; },
            [](int64_t arg) { std::cerr << "int64(" << arg << ")"; },
            [](const ValueString arg) {std::cerr << "ValueString(" << *arg << ")"; },
            [](const ValueCFunction arg) {std::cerr << "CFunction()"; },
            [](const ValueCMethod arg) {std::cerr << "CMethod()"; },
            [](const ValueCode arg) {std::cerr << "Code()"; },
            [](const ValuePyFunction arg) {std::cerr << "Python Function()"; },
            [](const ValuePyClass arg) {std::cerr << "ValuePyClass ("
                << *(std::get<ValueString>((*(arg->attrs))["__qualname__"])) << ")"; },
            [](const ValuePyObject arg) {std::cerr << "ValuePyObject of class ("
                << *(std::get<ValueString>((*(arg->static_attrs->attrs))["__qualname__"])) << ")"; },
            [](value::NoneType) {std::cerr << "None"; },
            [](bool val) {if (val) std::cerr << "bool(true)"; else std::cout << "bool(false)"; },
            [](ValueList value) {
                std::cerr << "[";
                for (Value& value : value->values) {
                    FrameState::print_value(value);
                    std::cerr << ",";
                }
                std::cerr << "]";
            }
        }, val);
}

void FrameState::print_stack() const {
    std::cerr << "[";
    for (size_t i = 0; i < this->value_stack.size(); ++i) {
        if (i != 0) {
            std::cerr << ", ";
        }
        std::cerr << i << "_";
        Value val = this->value_stack[i];
        FrameState::print_value(val);
    }
    std::cerr << "].len = " << this->value_stack.size();

    std::cerr << std::endl;
}

inline void FrameState::eval_next() {
    if (this->r_pc >= code->instructions.size()) {
        throw pyerror("overflowed instructions vector, no code here to run.");
    }

    Code::Instruction instruction = code->instructions[this->r_pc];
    const Code::ByteCode bytecode = instruction.bytecode;
    const uint64_t arg = instruction.arg;

    DEBUG("%03llu EVALUATE BYTECODE: %s", this->r_pc, op::name[bytecode])
    switch (bytecode) {
        case 0:
            break;
        case op::LOAD_GLOBAL:
            try {
                // Look for which name we are loading
                const std::string& name = this->code->co_names.at(arg);

                // Find it
                auto itr_local = this->interpreter_state->ns_globals->find(name);

                // Push it to the stack if it exists, otherwise try builtins
                if (itr_local != this->interpreter_state->ns_globals->end()) {
                    DEBUG("op::LOAD_GLOBAL ('%s') loaded a global", name.c_str());
                    this->value_stack.push_back(itr_local->second);
                    break;
                }
                
                // Try builtins
                auto itr_local_b = this->interpreter_state->ns_builtins->find(name);
                if (itr_local_b != this->interpreter_state->ns_builtins->end()){
                    DEBUG("op::LOAD_GLOBAL ('%s') loaded a builtin", name.c_str());
                    this->value_stack.push_back(itr_local_b->second);
                    break;
                }

                 throw pyerror(string("op::LOAD_GLOBAL name not found: ") + name);
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_FAST tried to load name out of range");
            }
            break ;
        case op::LOAD_FAST:
            try {
                // Look for which name we are loading
                const std::string& name = this->code->co_varnames.at(arg);

                // Find it
                auto itr_local = this->ns_local->find(name);

                // Push it to the stack if it exists, otherwise error
                if (itr_local != this->ns_local->end()) {
                    DEBUG("op::LOAD_FAST ('%s') loaded a local", name.c_str());
                    this->value_stack.push_back(itr_local->second);
                } else {
                    throw pyerror(string("op::LOAD_FAST name not found: ") + name);
                }
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_FAST tried to load name out of range");
            }
            break ;
        case op::LOAD_NAME:
        {
            try {
                const std::string& name = this->code->co_names.at(arg);
                const auto& globals = this->interpreter_state->ns_globals;
                const auto& builtins = this->interpreter_state->ns_builtins;
                auto itr_local = this->ns_local->find(name);
                if (itr_local != this->ns_local->end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a local", name.c_str());
                    this->value_stack.push_back(itr_local->second);
                    break ;
                } 
                auto itr_global = globals->find(name);
                if (itr_global != globals->end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a global", name.c_str());
                    this->value_stack.push_back(itr_global->second);
                    break ;
                } 
                auto itr_builtin = builtins->find(name);
                if (itr_builtin != builtins->end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a builtin", name.c_str());
                    this->value_stack.push_back(itr_builtin->second);
                    break ;
                } 
                
                throw pyerror(string("op::LOAD_NAME name not found: ") + name);
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_NAME tried to load name out of range");
            }
            break ;
        }
        case op::STORE_GLOBAL:
            this->check_stack_size(1);
            try {
                // Check which name we are storing and store it
                const std::string& name = this->code->co_names.at(arg);
                DEBUG_ADV("\top::STORE_GLOBAL set " << name << " = " << this->value_stack.back());
                (*(this->interpreter_state->ns_globals))[name] = std::move(this->value_stack.back());
                this->value_stack.pop_back();
            } catch (std::out_of_range& err) {
                throw pyerror("op::STORE_GLOBAL tried to store name out of range");
            }
            break;
        case op::STORE_FAST:
            this->check_stack_size(1);
            try {
                // Check which name we are storing and store it
                const std::string& name = this->code->co_varnames.at(arg);
                DEBUG_ADV("\top::STORE_FAST set " << name << " = " << this->value_stack.back());
                (*(this->ns_local))[name] = std::move(this->value_stack.back());
                this->value_stack.pop_back();
            } catch (std::out_of_range& err) {
                throw pyerror("op::STORE_FAST tried to store name out of range");
            }
            break;
        case op::STORE_NAME:
        {   
            this->check_stack_size(1);
            try {
                const std::string& name = this->code->co_names.at(arg);
                DEBUG_ADV("\top::STORE_NAME set " << name << " = " << this->value_stack.back());
                (*(this->ns_local))[name] = std::move(this->value_stack.back());
                this->value_stack.pop_back();
            } catch (std::out_of_range& err) {
                throw pyerror("op::STORE_NAME tried to store name out of range");
            }
            break;
        }
        case op::LOAD_CONST:
        {
            try {
                DEBUG("op::LOAD_CONST pushed constant at index %d", (int)arg);
                this->value_stack.push_back(
                    this->code->co_consts.at(arg)
                );
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_CONST tried to load constant out of range");
            }
            break ;
        }
        case op::CALL_FUNCTION:
        {
            DEBUG("op::CALL_FUNCTION attempted to call a function with %d arguments", arg);
            this->check_stack_size(1 + arg);
            
            // Instead of reading out into a vector, can I just pass have 'initialize_from_pyfunc'
            // read directly off the stack?
            std::vector<Value> args(
                this->value_stack.end() - arg,
                this->value_stack.end());
            this->value_stack.resize(this->value_stack.size() - arg);
            // note our usage of std::move, std::move denotes that rather than
            // copy constructing assignment, we should actually move the value 
            // and invalidate the source because the source will not be used 
            // in the future
            Value func = std::move(this->value_stack.back());
            this->value_stack.pop_back();
            
            std::visit(
                value_helper::call_visitor(*this, args),
                func
            );

            break ;
        }
        case op::POP_TOP:
        {
            this->check_stack_size(1);
            this->value_stack.pop_back();
            break ;
        }
        case op::ROT_TWO:
        {
            std::swap(*(this->value_stack.end()), *(this->value_stack.end() - 1));
            break ;
        }
        case op::COMPARE_OP:
        {
            this->check_stack_size(2);
            Value val2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value val1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            DEBUG("BEFORE COMPARISON STACK SIZE: %d",this->value_stack.size());
            DEBUG("\tCOMPARISON OPERATOR: %s", op::cmp::name[arg]);
            switch (arg) {
                case op::cmp::LT:
                    std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_lt>(*this),
                        val1, val2);
                    break;
                case op::cmp::LTE:
                    std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_lte>(*this),
                        val1, val2);
                    break;
                case op::cmp::GT:
                    std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_gt>(*this),
                        val1, val2);
                    break;
                case op::cmp::GTE:
                    std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_gte>(*this),
                        val1, val2);
                    break;
                case op::cmp::EQ:
                    std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_eq>(*this),
                        val1, val2);
                    break ;
                case op::cmp::NEQ:
                    std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_neq>(*this),
                        val1, val2);
                    break ;
                default:
                    throw pyerror(string("operator ") + op::cmp::name[arg] + " not implemented.");
            }
            DEBUG("AFTER COMPARISON STACK SIZE: %d",this->value_stack.size());
            break;
        }
        case op::INPLACE_ADD:
        // see https://stackoverflow.com/questions/15376509/when-is-i-x-different-from-i-i-x-in-python
        // INPLACE_ADD should call __iadd__ method on full objects, falls back to __add__ if not available.
        case op::BINARY_ADD:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::add_visitor(*this),v1,v2);
            break ;
        }
        case op::INPLACE_SUBTRACT:
        case op::BINARY_SUBTRACT:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_sub>(*this),v1,v2);
            break ;
        }
        case op::INPLACE_FLOOR_DIVIDE:
        case op::BINARY_FLOOR_DIVIDE:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_divide>(*this),v1,v2);
            break ;
        }
        case op::INPLACE_MULTIPLY:
        case op::BINARY_MULTIPLY:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_mult>(*this),v1,v2);
            break ;
        }
        case op::INPLACE_MODULO:
        case op::BINARY_MODULO:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_modulo>(*this),v1,v2);
            break ;
        }
        case op::RETURN_VALUE:
        {
            this->check_stack_size(1);
            switch(flags){
                case 0:
                    {
                        // Normal function return
                        auto val = this->value_stack.back();
                        if (this->parent_frame != nullptr) {
                            this->parent_frame->value_stack.push_back(std::move(val));
                        }
                        break;
                    }
                case FrameState::FLAG_CLASS_STATIC_INIT:
                case FrameState::FLAG_CLASS_DYNAMIC_INIT:
                    break;
                case FrameState::FLAG_IS_GENERATOR_FUNCTION:
                    // a false value indicates that the generator is depleated,
                    // a true value is simply popped and the value under it 
                    // is treated as the YIELD
                    this->parent_frame->value_stack.push_back(false);
                    break ;
                default:
                    throw pyerror("Invalid FrameState flags");
                    break;
            }

            // Pop the call stack
            this->interpreter_state->pop_frame();
            break ;
        }
        case op::SETUP_LOOP:
        {
            Block newBlock;
            newBlock.type = Block::Type::LOOP;
            newBlock.level = this->value_stack.size();
            newBlock.pc_start = this->code->instructions[this->r_pc + 1].bytecode_index; // GARETH: changed this offset to 1
            newBlock.pc_delta = arg;
            this->block_stack.push(newBlock);
            DEBUG("new block stack height: %lu", this->block_stack.size())
            break ;
        }
        case op::BREAK_LOOP: 
        {
            Block topBlock = this->block_stack.top();
            this->block_stack.pop();
            size_t jumpTo = this->code->pc_map[(topBlock.pc_start + topBlock.pc_delta)];
            if (jumpTo == 0) {
                throw pyerror("invalid jump destination for break loop!");
            }

            this->r_pc = jumpTo;
            return ;
        }
        case op::POP_BLOCK:
            this->block_stack.pop();
            break ;
        case op::POP_JUMP_IF_FALSE:
        {
            this->check_stack_size(1);
            // TODO: implement handling for truthy values.
            Value top = std::move(this->value_stack.back());
            this->value_stack.pop_back();

            if (!std::visit(value_helper::visitor_is_truthy(), top)) {
                this->r_pc = arg;
                return ;
            }
            break;
        }
        case op::JUMP_ABSOLUTE:
            this->r_pc = arg;
            return ;
        case op::JUMP_FORWARD:
            this->r_pc += arg;
            return ;
        case op::MAKE_FUNCTION:
        {
            this->check_stack_size(arg + 2);

            // Pop the name and code
            ValueString name;
            ValueCode code;
            try {
                name = std::move(std::get<ValueString>(value_stack.back()));
                this->value_stack.pop_back();
                code = std::move(std::get<ValueCode>(value_stack.back()));
                this->value_stack.pop_back();
            } catch (std::bad_variant_access& err) {
                std::stringstream ss;
                ss << "MAKE_FUNCTION expects function name to be a string, and code object"
                    " to be of type code object.";
                throw pyerror(ss.str());
            }

            // Create a shared pointer to a vector from the args
            std::shared_ptr<std::vector<Value>> v = std::make_shared<std::vector<Value>>(
                std::vector<Value>(this->value_stack.end() - arg, this->value_stack.end())
            );
            this->value_stack.resize(this->value_stack.size() - arg);

            DEBUG_ADV("Arguments:");
            #ifdef DEBUG_ON
            for(int i = 0; i < v->size(); i++) {
                DEBUG_ADV(i << " => " << (*v)[i]);
            }
            #endif

            this->value_stack.push_back(
                std::make_shared<value::PyFunc>(
                    value::PyFunc {name, code, v}
                )
            );
            break;
        }
        case op::LOAD_BUILD_CLASS:
        {
            // Push the build class builtin onto the stack
            J_DEBUG("Preparing to build a class");
            this->value_stack.push_back((*(this->interpreter_state->ns_builtins))["__build_class__"]);
            break;
        }
        case op::LOAD_ATTR:
        {
            DEBUG("Loading Attr %s",this->code->co_names[arg].c_str());
            Value val = std::move(value_stack.back());
            this->value_stack.pop_back();
            // Visit a load_attr_visitor constructed with the frame state and the arg to get
            // Do it this way because val might turn out to be a PyClass or a PyObject
            std::visit(
                value_helper::load_attr_visitor(*this,this->code->co_names[arg]),
                val
            );
            break;
        }
        case op::STORE_ATTR:
        {
            DEBUG("Storing Attr %s",this->code->co_names[arg].c_str()) ;
            
            Value tos = std::move(this->value_stack.back());
            this->value_stack.pop_back();
            Value val = std::move(this->value_stack.back());
            this->value_stack.pop_back();

            // This should probably be done with a visitor pattern
            // But That sounds to like alot more compile time for something thats honestly really simple
            auto vpo = std::get_if<ValuePyObject>(&tos);
            if(vpo != NULL){
                (*vpo)->store_attr(this->code->co_names[arg],val);
                break;
            }
            auto vpc = std::get_if<ValuePyClass>(&tos);
            if(vpc != NULL){
                (*vpc)->store_attr(this->code->co_names[arg],val);
                break;
            }

            // We should never get here
            throw pyerror(std::string("STORE_ATTR called with bad stack!"));
            break;
        }
        case op::BUILD_LIST:
        {
            this->check_stack_size(arg);

            // Pop the arguments to turn into a list.
            ValueList newList = this->interpreter_state->alloc.heap_lists.make();

            newList->values.assign(this->value_stack.end() - arg, this->value_stack.end());
            this->value_stack.resize(this->value_stack.size() - arg);
            
            this->value_stack.push_back(newList);

            break;
        }
        case op::BINARY_SUBSCR:
        {
            this->check_stack_size(2);
            Value index = std::move(this->value_stack.back());
            this->value_stack.pop_back();
            Value list = std::move(this->value_stack.back());
            this->value_stack.pop_back();

            this->value_stack.push_back(
                std::move(
                    std::visit(eval_helpers::binary_subscr_visitor(), list, index)
                )
            );

            break ;
        }
        case op::GET_ITER:
        {
            DEBUG_ADV("GET_ITER IS A NULL OP FOR NOW");
            break ;
        }
        case op::FOR_ITER:
        {
            DEBUG_ADV("\tJUMP OFFSET FOR ITERATOR: " << arg);
            std::visit(eval_helpers::for_iter_visitor {*this, arg}, this->value_stack.back());
            return ;
        }
        case op::YIELD_VALUE:
        {
            this->check_stack_size(1);

            if (this->get_flag(FrameState::FLAG_IS_GENERATOR_FUNCTION)) {
                // we pop a value from our stack, 
                Value val = std::move(this->value_stack.back());
                DEBUG_ADV("\tYIELDING VALUE: " << val);
                // we push that value to the parent_frame's stack, which was 
                // set by GET_ITER (we hope!)
                this->parent_frame->value_stack.push_back(std::move(val));
                this->parent_frame->value_stack.push_back(true);
                this->interpreter_state->pop_frame();

                break ;
            } else {
                this->set_flag(FrameState::FLAG_IS_GENERATOR_FUNCTION);
                DEBUG_ADV("\tENCOUNTERED op::YIELD_VALUE in function -- treating it as a generator"
                    ", moving frame off of linear call stack, and returning a generator object" 
                    " with that frame.");
                // we ran into an op::YIELD_VALUE, this means we need to go back
                // act as though this function call returned a genrator,
                // in reality we are giving that generator a reference to this
                // stack frame, and then removing this frame from the call stack
                // so that its execution can 'sortof' continue on the side

                // have the generator hold a reference to the current frame
                value::PyGenerator generator { this->interpreter_state->cur_frame };
                this->interpreter_state->pop_frame();
                // now that the interpreter state is holding our parent frame, null the parent 
                // frame as our 'parent frame' is now managed by GET_ITER
                generator.frame->parent_frame = nullptr;
                this->interpreter_state->cur_frame->value_stack.push_back(
                    std::move(generator)
                );
                DEBUG_ADV("\tframe was successfully completely moved from the callstack.");

                return ; // return so that this op will be run again.
            }
        }
        
        default:
        {
            std::stringstream ss;
            ss << "UNIMPLEMENTED BYTECODE: " << op::name[bytecode];
            std::cerr << ss.str() << std::endl;
            throw pyerror(ss.str());
        }
    }
    
    // REMINDER: some instructions like op::JUMP_ABSOLUTE return early, so they
    // do not reach this point
#ifdef DEBUG_STACK
    std::cerr << "\tSTACK AFTER op::" << op::name[bytecode] << ": ";
    this->print_stack();
#endif

    this->r_pc++;
}

void InterpreterState::eval() {
    try {
        while (this->cur_frame != nullptr) {
            this->cur_frame->eval_next();
        }
    } catch (const pyerror& err) {
        auto& frame = *(this->cur_frame);
        std::cerr << err.what() << std::endl;
        std::cerr << "FRAME TRACE: " << std::endl;

        FrameState *frm = &frame;
        std::string indent = "\t";
        while (frm != nullptr) {
            const auto& lnotab = frm->code->lnotab;
            for (size_t i = 1; i < lnotab.size(); ++i) {
                auto mapping = lnotab[i];
                if (i == lnotab.size() - 1) {
                    std::cerr << indent << frm->code->co_name << ":" << mapping.line << std::endl;
                    break ;
                } else if (mapping.pc >= frm->r_pc) {
                    std::cerr << indent << frm->code->co_name << ":" << lnotab[i - 1].line << std::endl;
                    break ;
                }
            }
            indent += "\t";
            frm = frm->parent_frame.get();
        }
        std::cerr << "STACK:";
        frame.print_stack();
        throw err;
    }
}

}