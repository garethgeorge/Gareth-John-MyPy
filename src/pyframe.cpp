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

FrameState::FrameState(
        InterpreterState *interpreter_state, 
        FrameState *parent_frame,
        const ValueCode& code) 
{
    DEBUG("constructed a new frame");
    this->ns_local = std::make_shared<std::unordered_map<std::string, Value>>();
    this->interpreter_state = interpreter_state;
    this->parent_frame = parent_frame;
    this->code = code;
    DEBUG("reserved %lu bytes for the stack", code->co_stacksize);
    this->value_stack.reserve(code->co_stacksize);
}

// Construct a framestate meant to initialize everything static about a class
FrameState::FrameState(
        InterpreterState *interpreter_state, 
        FrameState *parent_frame,
        const ValueCode& code,
        ValuePyClass& init_class)
{
    DEBUG("constructed a new frame for statically initializing a class");
    // Everything is mostly the same, but our local namespace is also the class's
    this->interpreter_state = interpreter_state;
    this->parent_frame = parent_frame;
    this->code = code;
    DEBUG("reserved %lu bytes for the stack", code->co_stacksize);
    this->value_stack.reserve(code->co_stacksize);
    this->init_class = init_class;
    this->ns_local = this->init_class->attrs;
    this->flags |= CLASS_INIT_FRAME;
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

void FrameState::print_next() {
    Code::ByteCode bytecode = code->bytecode[this->r_pc];
    if (this->r_pc >= this->code->bytecode.size()) {
        printf("popped a frame from the call stack");
        this->interpreter_state->callstack.pop();
        return ;
    }

    if (bytecode == 0) {
        this->r_pc++;
        return ;
    }

    printf("%10llu %s\n", this->r_pc, op::name[bytecode]);
    if (bytecode == op::LOAD_CONST) {
        // printf("\tconstant: %s\n", ((std::string)code->co_consts[code->bytecode[this->r_pc + 1]]).c_str());
    }
    
    if (bytecode < op::HAVE_ARGUMENT) {
        this->r_pc += 1;
    } else {
        this->r_pc += 2;
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
        
        void operator()(const std::shared_ptr<std::string>& v1, const std::shared_ptr<std::string> &v2) const {
            frame.value_stack.push_back(std::make_shared<std::string>(*v1 + *v2));
        }
    };

    // Visitor for accessing class attributes
    struct load_attr_visitor {
        FrameState& frame;
        const std::string& attr;

        load_attr_visitor(FrameState& frame, const std::string& attr) : frame(frame), attr(attr) {}
        
        void operator()(const ValuePyClass& cls){
            try {
                frame.value_stack.push_back(cls->attrs->at(attr));
            } catch (const std::out_of_range& oor) {
                throw pyerror(std::string(
                    *(std::get<ValueString>( (*(cls->attrs))["__qualname__"]))
                    + " has no attribute " + attr
                ));
            }
        }

        // For pyobject, first look in their own namespace, then look in their static namespace
        // ValuePyObject cannot be const as it might be modified
        void operator()(ValuePyObject& obj){
            // First look in my own namespace
            std::tuple<Value,bool> res = value::PyObject::find_attr_in_obj(obj, attr);
            if(std::get<1>(res)){
                frame.value_stack.push_back(std::get<0>(res));
            } else {
                // Nothing found, throw error!
                throw pyerror(std::string(
                    // Should this be __name__??
                    *(std::get<ValueString>( (*(obj->static_attrs->attrs))["__qualname__"]))
                    + " has no attribute " + attr
                ));
            }
        }

        template<typename T>
        void operator()(T) const {
            throw pyerror(string("can not get attributed from an object of type ") + typeid(T).name());
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

void FrameState::initialize_from_pyfunc(const ValuePyFunction func, std::vector<Value>& args){
    // Set current function
    curr_func = func;
    
    // Calculate which argument is the first argument with a default value
    // Also whether or not the very first argument is self (or class)
    // This could be stored in PyFunc struct but that is a tiny space tradeoff vs tiny time tradeoff
    int first_def_arg = this->code->co_argcount - func->def_args->size();
   
    // Set the implicit argument
    bool has_implicit_arg = func->flags & (value::CLASS_METHOD | value::INSTANCE_METHOD);
    if(has_implicit_arg) std::visit(set_implicit_arg_visitor(*this),func->self);

    int first_arg_is_self = (has_implicit_arg ? 1 : 0);


    /*J_DEBUG("Cell Vars:\n")
    for(int i = 0;i < this->code->co_cellvars.size();i++){
        std::cout << this->code->co_cellvars[i] << "\n";
    }*/

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

        if(this->code->co_cellvars.size() == 0){
            // The argument exists, save it
            add_to_ns_local(
                // Read the name to save to from the constants pool
                this->code->co_varnames[i], 
                // read the value from passed in args, or else the default
                arg_num < args.size() ? std::move(args[arg_num]) : (*(func->def_args))[arg_num - first_def_arg] 
            );
        } else {
            // I haaate this copy/paste
            Value v = arg_num < args.size() ? std::move(args[arg_num]) : (*(func->def_args))[arg_num - first_def_arg];
            
            bool found = false;
            // Check to see if this is a cell var
            for(auto it = this->code->co_cellvars.begin();it != this->code->co_cellvars.end();++it){
                if((*it).compare(this->code->co_varnames[i]) == 0){
                    add_to_ns_local(
                        this->code->co_varnames[i], 
                        std::move(value_helper::create_cell(v))
                    );
                    found = true;
                    break;
                }
            }
            if(!found){
                add_to_ns_local(
                        this->code->co_varnames[i], 
                        std::move(v)
                    );
            }
        }
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

// If TOS supports an inplace operation, do it
bool attempt_inplace_op(FrameState& frame,const std::string& i_attr){
        // Get the TOS and check if it's and object
        frame.check_stack_size(2);
        Value v1 = frame.value_stack[frame.value_stack.size() - 2];
        auto obj = std::get_if<ValuePyObject>(&v1);
        if(obj != NULL){

            // Check if it  overloaded the attr
            std::tuple<Value,bool> res = (*obj)->find_attr_in_obj((*obj),i_attr);
            
            if(std::get<1>(res)) {
                // It did!
                // Call the function
                std::vector<Value> args; 
                args.push_back(std::move(frame.value_stack[frame.value_stack.size() - 1]));
                frame.value_stack.resize(frame.value_stack.size() - 2);
                std::visit(
                    value_helper::call_visitor(frame,args),
                    std::get<0>(res)
                );
                return true;
            }

            // Default to non-inplace
            return false;
        }
        return false;

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
    Code::ByteCode bytecode = code->bytecode[this->r_pc];
    
    // Read the argument
    uint64_t arg = code->bytecode[this->r_pc + 1] | (code->bytecode[this->r_pc + 2] << 8);

    // Extend it if necessary
    while(bytecode == op::EXTENDED_ARG){
        this->r_pc += 3;
        bytecode = code->bytecode[this->r_pc];
        arg = (arg << 16) | code->bytecode[this->r_pc + 1] | (code->bytecode[this->r_pc + 2] << 8);
    }
    
    if (this->r_pc >= this->code->bytecode.size()) {
        DEBUG("overflowed program, popped stack frame, however this indicates a failure so we will exit.");
        this->interpreter_state->callstack.pop();
        exit(0);
        return ;
    }

    DEBUG("%03llu EVALUATE BYTECODE: %s", this->r_pc, op::name[bytecode])
    switch (bytecode) {
        case 0:
            break;
        case op::LOAD_GLOBAL:
            try {
                // Look for which name we are loading
                const std::string& name = this->code->co_names.at(arg);

                // Find it
                auto itr_local = this->interpreter_state->ns_globals_ptr->find(name);

                // Push it to the stack if it exists, otherwise try builtins
                if (itr_local != this->interpreter_state->ns_globals_ptr->end()) {
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
        case op::LOAD_CLOSURE:
            try {
                std::string name;
                if(arg < this->code->co_cellvars.size()){
                    name = this->code->co_cellvars.at(arg);
                } else {
                    name = this->code->co_freevars.at(arg - this->code->co_cellvars.size());
                }
                const auto& globals = this->interpreter_state->ns_globals_ptr;
                const auto& builtins = this->interpreter_state->ns_builtins;
                // Search through all local namespaces up the stack
                auto curr_frame = this;
                bool found = false;
                while(curr_frame != NULL){
                    auto itr_local = curr_frame->ns_local->find(name);
                    if (itr_local != curr_frame->ns_local->end()) {
                        DEBUG("op::LOAD_CLOSURE ('%s') loaded a local", name.c_str());
                        // This is expected to be a cell
                        this->value_stack.push_back(itr_local->second);
                        found = true;
                        curr_frame = NULL;
                        break ;
                    } 
                    curr_frame = curr_frame->parent_frame;
                }
                // Do not check globals or builtins for free vars
                if(!found){
                    throw pyerror(string("op::LOAD_CLOSURE name not found: ") + name);
                }
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_CLOSURE tried to load name out of range");
            }
            break;
        case op::LOAD_DEREF:
        {
            // Access the closure of the function
            if(arg >= this->curr_func->__closure__->values.size()){
                throw pyerror("Attempted LOAD_DEREF out of range\n");
            } else {
                DEBUG("SIZE: %d\n",this->curr_func->__closure__->values.size());
                // Push to the top of the stack the contents of cell arg in the current enclosing scope
                this->value_stack.push_back(
                    std::get<ValuePyObject>(this->curr_func->__closure__->values[arg])->attrs->at("contents")
                );
            }
            break;
        }
        case op::LOAD_NAME:
        {
            try {
                const std::string& name = this->code->co_names.at(arg);
                const auto& globals = this->interpreter_state->ns_globals_ptr;
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
            break;
        }
        case op::STORE_GLOBAL:
            this->check_stack_size(1);
            try {
                // Check which name we are storing and store it
                const std::string& name = this->code->co_names.at(arg);
                (*(this->interpreter_state->ns_globals_ptr))[name] = std::move(this->value_stack.back());
                this->value_stack.pop_back();
            } catch (std::out_of_range& err) {
                throw pyerror("op::STORE_GLOBAL tried to store name out of range");
            }
            break;
        case op::STORE_FAST:
            this->check_stack_size(1);
            try {
                //DEBUG("STORE_FAST ARG: %d\n",arg);
                // Check which name we are storing and store it
                const std::string& name = this->code->co_varnames.at(arg);
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
            if(attempt_inplace_op(*this,"__iadd__")) break;
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
            if(attempt_inplace_op(*this,"__isub__")) break;
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
            if(attempt_inplace_op(*this,"__ifloordiv__")) break;
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
            if(attempt_inplace_op(*this,"__imul__")) break;
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
            if(attempt_inplace_op(*this,"__imod__")) break;
        case op::BINARY_MODULO:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_modulo>(*this),v1,v2);
            break ;
        }
        case op::INPLACE_POWER:
            if(attempt_inplace_op(*this,"__ipow__")) break;
        case op::BINARY_POWER:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_pow>(*this),v1,v2);
            break ;
        }
        case op::INPLACE_TRUE_DIVIDE:
            if(attempt_inplace_op(*this,"__itruediv__")) break;
        case op::BINARY_TRUE_DIVIDE:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_true_div>(*this),v1,v2);
            break ;
        }
        case op::INPLACE_LSHIFT:
            if(attempt_inplace_op(*this,"__ilshift__")) break;
        case op::BINARY_LSHIFT:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_lshift>(*this),v1,v2);
            break ;
        }
        case op::INPLACE_RSHIFT:
            if(attempt_inplace_op(*this,"__irshift__")) break;
        case op::BINARY_RSHIFT:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_rshift>(*this),v1,v2);
            break ;
        }
        case op::INPLACE_AND:
            if(attempt_inplace_op(*this,"__iand__")) break;
        case op::BINARY_AND:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_and>(*this),v1,v2);
            break;
        }
        case op::INPLACE_XOR:
            if(attempt_inplace_op(*this,"__ixor__")) break;
        case op::BINARY_XOR:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_xor>(*this),v1,v2);
            break;
        }
        case op::INPLACE_OR:
            if(attempt_inplace_op(*this,"__ior__")) break;
        case op::BINARY_OR:
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_or>(*this),v1,v2);
            break;
        }
        case op::RETURN_VALUE:
        {
            this->check_stack_size(1);
            if((this->flags & (CLASS_INIT_FRAME | OBJECT_INIT_FRAME | DONT_RETURN_FRAME)) == 0){
                // Normal function return
                auto val = this->value_stack.back();
                if (this->parent_frame != nullptr) {
                    this->parent_frame->value_stack.push_back(std::move(val));
                }
            }
                
            // Pop the call stack
            this->interpreter_state->callstack.pop();
            break ;
        }
        case op::SETUP_LOOP:
        {
            Block newBlock;
            newBlock.type = Block::Type::LOOP;
            newBlock.level = this->value_stack.size();
            newBlock.pc_start = this->r_pc + 2;
            newBlock.pc_delta = arg;
            this->block_stack.push(newBlock);
            DEBUG("new block stack height: %lu", this->block_stack.size())
            break ;
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
            try {
                if (!std::get<bool>(top)) {
                    this->r_pc = arg;
                    return ;
                }
            } catch (std::bad_variant_access& e) {
                // TODO: does not actually matter what the type of the condition is :o
                throw pyerror("expected condition to have type bool, got bad type.");
            }
            break;
        }
        case op::JUMP_ABSOLUTE:
            this->r_pc = arg;
            return ;
        case op::JUMP_FORWARD:
            this->r_pc += arg;
            return ;
        case op::MAKE_CLOSURE:
        {
            // Loooots of copy/paste here
            // I really should factor the copied part of make_closure/make_function into a function,
            // But since I will just undo that later for direct threading i guess it's copy/paste time
            this->check_stack_size(arg + 3);

            // Pop the name and code
            Value name = std::move(value_stack.back());
            this->value_stack.pop_back();
            Value code = std::move(value_stack.back());
            this->value_stack.pop_back();
            //ValueList closure = std::move(std::get<ValueList>(value_stack.back()));
            ValueList closure = std::get<ValueList>(value_stack.back());
            this->value_stack.pop_back();

            // Create a shared pointer to a vector from the args
            std::shared_ptr<std::vector<Value>> v = std::make_shared<std::vector<Value>>(
                std::vector<Value>(this->value_stack.end() - arg, this->value_stack.end())
            );
            
            // Remove the args from the value stack
            this->value_stack.resize(this->value_stack.size() - arg);
            // Create the function object
            // Error here if the wrong types
            try {
                ValuePyFunction nv = std::make_shared<value::PyFunc>(
                    value::PyFunc {std::get<ValueString>(name), std::get<ValueCode>(code), v}
                );
                // CHange to a tuple!
                nv->__closure__ = closure;
                this->value_stack.push_back(nv);
            } catch (std::bad_variant_access&) {
                std::stringstream ss;
                ss << "MAKE FUNCTION called with name '" << name << "' and code block: " << code;
                ss << ", but make function expects string and code object";
                throw pyerror(ss.str());
            }
            break;
        }
        case op::MAKE_FUNCTION:
        {

            // DONT FORGET TO CHANGE MAKE_CLOSURE TOO WHEN YOU CHANGE HOW ARG IS USED HERE

            this->check_stack_size(arg + 2);

            // Pop the name and code
            Value name = std::move(value_stack.back());
            this->value_stack.pop_back();
            Value code = std::move(value_stack.back());
            this->value_stack.pop_back();

            // Create a shared pointer to a vector from the args
            std::shared_ptr<std::vector<Value>> v = std::make_shared<std::vector<Value>>(
                std::vector<Value>(this->value_stack.end() - arg, this->value_stack.end())
            );
            
            // Remove the args from the value stack
            this->value_stack.resize(this->value_stack.size() - arg);

            J_DEBUG("Creating a function %s that accepts %d default args:\n",*(std::get<ValueString>(name)),arg);
            J_DEBUG("Those default args are:\n");
            #ifdef JOHN_DEBUG_ON
            for(int i = 0;i < v->size();i++){
                print_value((*v)[i]);
                fprintf(stderr,"\n",arg);
            }
            #endif
            // Create the function object
            // Error here if the wrong types
            try {
                ValuePyFunction nv = std::make_shared<value::PyFunc>(
                    value::PyFunc {std::get<ValueString>(name), std::get<ValueCode>(code), v}
                );
                this->value_stack.push_back(nv);
            } catch (std::bad_variant_access&) {
                std::stringstream ss;
                ss << "MAKE FUNCTION called with name '" << name << "' and code block: " << code;
                ss << ", but make function expects string and code object";
                throw pyerror(ss.str());
            }
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
            DEBUG("Loading Attr %s",this->code->co_names[arg].c_str()) ;
            Value val = std::move(value_stack.back());
            this->value_stack.pop_back();
            // Visit a load_attr_visitor constructed with the frame state and the arg to get
            // Do it this way because val might turn out to be a PyClass or a PyObject
            std::visit(
                eval_helpers::load_attr_visitor(*this,this->code->co_names[arg]),
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
        case op::BUILD_TUPLE:
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
        default:
        {
            std::stringstream ss;
            ss << "UNIMPLEMENTED BYTECODE" << op::name[bytecode];
            DEBUG(ss.str().c_str());
            throw pyerror(ss.str());
        }
    }
    
    // REMINDER: some instructions like op::JUMP_ABSOLUTE return early, so they
    // do not reach this point
#ifdef DEBUG_STACK
    std::cerr << "\tSTACK AFTER op::" << op::name[bytecode] << ": ";
    this->print_stack();
#endif

    if (bytecode < op::HAVE_ARGUMENT) {
        this->r_pc += 1;
    } else {
        this->r_pc += 3;
    }
}

void InterpreterState::eval() {
    try {
        while (!this->callstack.empty()) {
            // TODO: try caching the top of the stack
            #ifdef PRINT_OPS
            this->callstack.top().eval_print();
            #else
            this->callstack.top().eval_next();
            #endif
        }
    } catch (const pyerror& err) {
        const auto& frame = this->callstack.top();
        std::cout << err.what() << std::endl;
        // std::cout << "ENCOUNTERED ERROR WHILE EVALUATING OPERATION: " 
        //     << op::name[frame.code->bytecode[frame.r_pc]] << std::endl;
        std::cout << "FRAME TRACE: " << std::endl;

        FrameState *frm = &this->callstack.top();
        std::string indent = "\t";
        while (frm != nullptr) {
            const auto& lnotab = frm->code->lnotab;
            for (size_t i = 1; i < lnotab.size(); ++i) {
                auto mapping = lnotab[i];
                if (i == lnotab.size() - 1) {
                    std::cout << indent << frm->code->co_name << ":" << mapping.line << std::endl;
                    break ;
                } else if (mapping.pc >= frm->r_pc) {
                    std::cout << indent << frm->code->co_name << ":" << lnotab[i - 1].line << std::endl;
                    break ;
                }
            }
            indent += "\t";
            frm = frm->parent_frame;
        }
        std::cout << "STACK:";
        frame.print_stack();
        throw err;
    }
}

}