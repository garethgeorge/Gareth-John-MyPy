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

#define DIRECT_THREADED
#ifdef DIRECT_THREADED
    #define CONTEXT_SWITCH break;
    #define CONTEXT_SWITCH_KEEP_PC return;
    // Add a label after each case
    #define CASE(arg) case op::arg:\
                      arg:
    // The hint below means we expect to always be the top frame
    // That's not great at all, but this leaves things like BINARY_ADD
    // untouched (they onyl create a new frame when they happen to call an overload in a class)
    #define CONTEXT_SWITCH_IF_NEEDED \
        if(__builtin_expect(!(this->interpreter_state->cur_frame.get() == this),0)) break;

    #define GOTO_NEXT_OP \
    this->r_pc++;\
    instruction = code->instructions[this->r_pc];\
    bytecode = instruction.bytecode;\
    arg = instruction.arg;\
    DEBUG("%03llu EVALUATE BYTECODE: %s", this->r_pc, op::name[bytecode])\
    goto *jmp_table[bytecode];

    // Basically the same, but without incrementing pc
    #define GOTO_TARGET_OP \
    instruction = code->instructions[this->r_pc];\
    bytecode = instruction.bytecode;\
    arg = instruction.arg;\
    DEBUG("%03llu EVALUATE BYTECODE AFTER JUMP: %s", this->r_pc, op::name[bytecode])\
    goto *jmp_table[bytecode];

#else
    #define GOTO_NEXT_OP break;
    #define GOTO_TARGET_OP return;
    #define CONTEXT_SWITCH break;
    #define CONTEXT_SWITCH_KEEP_PC return;
    #define CONTEXT_SWITCH_IF_NEEDED
    #define CASE(arg) case op::arg:
#endif

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
    this->set_flag(FrameState::FLAG_CLASS_INIT_FRAME);
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

void FrameState::initialize_from_pyfunc(ValuePyFunction func, ArgList& args){
    // Set current function
    curr_func = func;
    DEBUG_ADV("Setting up stackframe from " << Value(func));
    DEBUG_ADV("Arguments passed to function: ");
    #ifdef DEBUG 
    for (size_t i = 0; i < args.size(); ++i) {
        DEBUG_ADV("\t" << i << ") " << args[i]);
    }
    #endif


    size_t argcount = this->code->co_argcount;
    if (func->flags & (value::INSTANCE_METHOD | value::STATIC_METHOD)) {
        argcount++;
    }

    // compute the index of the first argument that should be treated as a
    // default argument parameter

    bool has_implicit_arg = func->flags & (value::CLASS_METHOD | value::INSTANCE_METHOD);
    if (has_implicit_arg) {
        DEBUG_ADV("calling a class method! binding func->self as thisArg");
        args.bind(func->self);
    }


    if (args.size() < this->code->co_argcount - func->def_args->size()) {
        int missing_num = (this->code->co_argcount - func->def_args->size()) - args.size();
        DEBUG_ADV("Found that we are missing " << missing_num << " arguments, preparing and then throwing error.");
        std::stringstream ss;
        ss << "TypeError: " << Value(func) << " missing " << (missing_num) << " required positional arguments:";
        // List the missing params
        throw pyerror(ss.str());
    }

    DEBUG_ADV("Useful values to keep in mind:" 
        << "\n\thas_implicit_arg: " << has_implicit_arg 
        << "\n\targcount: " << argcount
        << "\n\tco_argcount: " << this->code->co_argcount
        << "\n\tdef_args->size(): " << func->def_args->size()
        << "\n\tfunc flags: " << func->flags);

    

    bool have_cells = this->code->co_cellvars.size() > 0;
    this->cells.resize(this->code->co_cellvars.size());

    DEBUG_ADV("Determined we have " << this->code->co_cellvars.size());

    DEBUG_ADV("Assigning arguments that do not have default values");
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& varname = this->code->co_varnames[i];
        const Value v = args[i];

        DEBUG_ADV("\t" << i << ") assigning '" << varname << "' = '" << v << "'");

        if (have_cells && (this->code->co_cellmap.find(varname) != this->code->co_cellmap.end())) {
            ValuePyObject new_cell = value_helper::create_cell(v);
            this->ns_local->emplace(varname, new_cell);
            this->cells[i] = new_cell;
        } else {
            this->ns_local->emplace(varname, v);
        }
    }

    DEBUG_ADV("Assigning arguments that DO have default values");
    DEBUG_ADV("first we will dump default arguments:");
    #ifdef DEBUG
    for (const auto& defarg : *(func->def_args)) {
        DEBUG_ADV("\t" << defarg);
    }
    #endif

    if (func->def_args->size() != 0) {
        // TODO: rewrite how default argument offsets are calculated
        // this is pretty terrible
        size_t first_def_arg = func->code->co_argcount - func->def_args->size();

        for (size_t i = args.size(); i < this->code->co_argcount; ++i) {
            const std::string& varname = this->code->co_varnames[i];
            int offset = i - first_def_arg;
            DEBUG_ADV("calculated offset: " << offset);
            Value default_value = (*(func->def_args))[offset];

            DEBUG_ADV("\t" << i << ") assigning '" << varname << "' = '" << default_value << "'");

            if (have_cells && this->code->co_cellmap.find(varname) != this->code->co_cellmap.end()) {
                ValuePyObject new_cell = value_helper::create_cell(default_value);
                this->ns_local->emplace(varname, new_cell);

                this->cells[i] = new_cell;
            } else {
                this->ns_local->emplace(varname, default_value);
            }
        }
    }
    

    // for(size_t i = 0; i < this->code->co_argcount; i++){
    //     //Error if not given enough arguments
    //     //TypeError: simplefunc() missing 2 required positional arguments: 'a' and 'd'
    //     if(i < first_def_arg && arg_num >= args.size()){
            
    //         return;
    //     }

    //     J_DEBUG("Name: %s\n",this->code->co_varnames[i].c_str());
    //     J_DEBUG("Value: ");
    //     #ifdef JOHN_DEBUG_ON
    //     print_value(arg_num < args.size() ? args[arg_num] : (*(func->def_args))[arg_num - first_def_arg]);
    //     #endif

    //     if(this->code->co_cellvars.size() == 0){
    //         // The argument exists, save it
    //         add_to_ns_local(
    //             // Read the name to save to from the constants pool
    //             this->code->co_varnames[i], 
    //             // read the value from passed in args, or else the default
    //             arg_num < args.size() ? std::move(args[arg_num]) : (*(func->def_args))[arg_num - first_def_arg] 
    //         );
    //     } else {
    //         // I haaate this copy/paste
    //         Value v = arg_num < args.size() ? std::move(args[arg_num]) : (*(func->def_args))[arg_num - first_def_arg];
            
    //         bool found = false;
    //         // Check to see if this is a cell var
    //         for(auto it = this->code->co_cellvars.begin();it != this->code->co_cellvars.end();++it){
    //             if((*it).compare(this->code->co_varnames[i]) == 0){
    //                 ValuePyObject new_cell = value_helper::create_cell(v);
    //                 add_to_ns_local(
    //                     this->code->co_varnames[i], 
    //                     new_cell
    //                 );
    //                 cells.push_back(new_cell);
    //                 found = true;
    //                 break;
    //             }
    //         }
    //         if(!found){
    //             add_to_ns_local(
    //                     this->code->co_varnames[i], 
    //                     std::move(v)
    //                 );
    //         }
    //     }
    //     ++args;
    // }
}

// Add a value to the ns local
void FrameState::add_to_ns_local(const std::string& name, Value&& v){
    this->ns_local->emplace(name,v);
}

void FrameState::print_value(Value& val) {
    std::visit(value_helper::overloaded {
            [](auto&& arg) { 
                //throw pyerror(string("unimplemented stack printer for stack value: ") + typeid(arg).name());
               std::cerr << (string("unimplemented stack printer for stack value: ") + typeid(arg).name());
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
                // Call the function
                // std::vector<Value> args;
                // args.push_back(std::move(frame.value_stack[frame.value_stack.size() - 1]));
                // frame.value_stack.resize(frame.value_stack.size() - 2);
                // ArgList arglist(std::move(args));
                // std::visit(
                //     value_helper::call_visitor(frame, arglist),
                //     std::get<0>(res)
                // );
                // throw pyerror("inplace op not implemented since ArgList refactor.");

                ArgList args(frame.value_stack.back());
                frame.value_stack.pop_back();
                args.bind(frame.value_stack.back());
                frame.value_stack.pop_back();
                std::visit(
                    value_helper::call_visitor(frame, args),
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

    // If direct threading, this holds the table of jumps!
    #ifdef DIRECT_THREADED
    #include "jmp_table.hpp"
    #endif

    if (this->r_pc >= code->instructions.size()) {
        throw pyerror("overflowed instructions vector, no code here to run.");
    }

    Code::Instruction instruction = code->instructions[this->r_pc];
    Code::ByteCode bytecode = instruction.bytecode;
    uint64_t arg = instruction.arg;

    DEBUG("%03llu EVALUATE BYTECODE: %s", this->r_pc, op::name[bytecode])
    switch (bytecode) {
        CASE(NOP)
            GOTO_NEXT_OP;
        CASE(LOAD_GLOBAL)
            try {
                // Look for which name we are loading
                const std::string& name = this->code->co_names.at(arg);

                // Find it
                auto itr_local = this->interpreter_state->ns_globals->find(name);

                // Push it to the stack if it exists, otherwise try builtins
                if (itr_local != this->interpreter_state->ns_globals->end()) {
                    DEBUG("op::LOAD_GLOBAL ('%s') loaded a global", name.c_str());
                    this->value_stack.push_back(itr_local->second);
                    GOTO_NEXT_OP;
                }
                
                // Try builtins
                auto itr_local_b = this->interpreter_state->ns_builtins->find(name);
                if (itr_local_b != this->interpreter_state->ns_builtins->end()){
                    DEBUG("op::LOAD_GLOBAL ('%s') loaded a builtin", name.c_str());
                    this->value_stack.push_back(itr_local_b->second);
                    GOTO_NEXT_OP;
                }

                 throw pyerror(string("op::LOAD_GLOBAL name not found: ") + name);
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_FAST tried to load name out of range");
            }
            GOTO_NEXT_OP ;
        CASE(LOAD_FAST)
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
            GOTO_NEXT_OP ;
        CASE(LOAD_CLOSURE)
            try {
                std::string name;
                if(arg < this->code->co_cellvars.size()){
                    name = this->code->co_cellvars.at(arg);
                } else {
                    name = this->code->co_freevars.at(arg - this->code->co_cellvars.size());
                }
                const auto& globals = this->interpreter_state->ns_globals;
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
                    curr_frame = curr_frame->parent_frame.get();
                }
                // Do not check globals or builtins for free vars
                if(!found){
                    throw pyerror(string("op::LOAD_CLOSURE name not found: ") + name);
                }
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_CLOSURE tried to load name out of range");
            }
            GOTO_NEXT_OP;
        CASE(LOAD_CLASSDEREF)
        {
            // First check the locals, otherwise fall through into LOAD_DEREF
            try {
                std::string name;
                if(arg < this->code->co_cellvars.size()){
                    name = this->code->co_cellvars.at(arg);
                } else {
                    name = this->code->co_freevars.at(arg - this->code->co_cellvars.size());
                }
                auto itr_local = this->ns_local->find(name);
                if (itr_local != this->ns_local->end()) {
                    DEBUG("op::LOAD_CLASSDEREF ('%s') loaded a local", name.c_str());
                    // This is expected to be a cell
                    this->value_stack.push_back(itr_local->second);
                    GOTO_NEXT_OP ;
                } else {
                    DEBUG("op::LOAD_CLASSDEREF did not find ('%s') locally, falling through...", name.c_str());
                }
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_CLASSDEREF tried to load name out of range");
            }
        }
        CASE(LOAD_DEREF)
        {
            auto which_frame = this;
            if(this->flags & FLAG_CLASS_INIT_FRAME) {
                which_frame = this->parent_frame.get();
            }

            // Check out of range
            if(arg >= which_frame->cells.size()){
                if(which_frame->curr_func && which_frame->curr_func->__closure__ != nullptr){
                    if((arg - which_frame->cells.size()) >=  which_frame->curr_func->__closure__->values.size()){
                        throw pyerror(std::string(
                            "Attempted LOAD_DEREF out of range 2 (" + std::to_string(arg) + ")\n"
                        ));
                    }
                } else {
                    throw pyerror(std::string(
                        "Attempted LOAD_DEREF out of range (" + std::to_string(arg) + ")\n"
                    ));
                }
            } 
            
            DEBUG("Accessing Cell %d",arg);
            DEBUG("Cells: %d",which_frame->cells.size());
            if(arg >= cells.size()){
                DEBUG("Function Closure: %d",which_frame->curr_func->__closure__->values.size());
            }

            // Access the closure of the function or the cells
            if(arg < which_frame->cells.size()){
                this->value_stack.push_back(
                    which_frame->cells[arg]->attrs->at("contents")
                );
            } else {
                // Push to the top of the stack the contents of cell arg in the current enclosing scope
                DEBUG_ADV("Here are some things: "   
                    << which_frame->curr_func << ","
                    << which_frame->curr_func->__closure__ << ","
                    << which_frame->curr_func->__closure__->values[arg] << ","
                    << std::get<ValuePyObject>(which_frame->curr_func->__closure__->values[arg]) << ","
                    << (*(std::get<ValuePyObject>(which_frame->curr_func->__closure__->values[arg])->attrs))["contents"] << "\n"
                );
                this->value_stack.push_back(
                    std::get<ValuePyObject>(which_frame->curr_func->__closure__->values[arg])->attrs->at("contents")
                );
            }
            GOTO_NEXT_OP;
        }
        CASE(STORE_DEREF)
        {
            this->check_stack_size(1);

            // Access the closure of the function or the cells
            if(arg < this->cells.size()){
                    (*(this->cells[arg]->attrs))["contents"] = std::move(this->value_stack.back());
                    this->value_stack.pop_back();
            } else {
                // If the function does not have a closure yet, give it one
                if(this->curr_func){
                    if(this->curr_func->__closure__ == nullptr){
                        this->curr_func->__closure__ = this->interpreter_state->alloc.heap_lists.make();
                    }
                    while(this->curr_func->__closure__->values.size() <= arg){
                        this->curr_func->__closure__->values.push_back(
                            value_helper::create_cell(value::NoneType())
                        );
                    }
                } else {
                    throw pyerror("Attempted STORE_DEREF out of range\n");
                }
                // Push to the top of the stack the contents of cell arg in the current enclosing scope
                (*(std::get<ValuePyObject>(this->curr_func->__closure__->values[arg])->attrs))["contents"]
                                                                    = std::move(this->value_stack.back());
                this->value_stack.pop_back();  
            }
            GOTO_NEXT_OP;
        }
        CASE(LOAD_NAME)
        {
            try {
                const std::string& name = this->code->co_names.at(arg);
                const auto& globals = this->interpreter_state->ns_globals;
                const auto& builtins = this->interpreter_state->ns_builtins;
                auto itr_local = this->ns_local->find(name);
                if (itr_local != this->ns_local->end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a local", name.c_str());
                    this->value_stack.push_back(itr_local->second);
                    GOTO_NEXT_OP ;
                } 
                auto itr_global = globals->find(name);
                if (itr_global != globals->end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a global", name.c_str());
                    this->value_stack.push_back(itr_global->second);
                    GOTO_NEXT_OP ;
                } 
                auto itr_builtin = builtins->find(name);
                if (itr_builtin != builtins->end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a builtin", name.c_str());
                    this->value_stack.push_back(itr_builtin->second);
                    GOTO_NEXT_OP ;
                } 
                
                throw pyerror(string("op::LOAD_NAME name not found: ") + name);
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_NAME tried to load name out of range");
            }
            GOTO_NEXT_OP;
        }
        CASE(STORE_GLOBAL)
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
            GOTO_NEXT_OP;
        CASE(STORE_FAST)
            this->check_stack_size(1);
            try {
                //DEBUG("STORE_FAST ARG: %d\n",arg);
                // Check which name we are storing and store it
                const std::string& name = this->code->co_varnames.at(arg);
                DEBUG_ADV("\top::STORE_FAST set " << name << " = " << this->value_stack.back());
                (*(this->ns_local))[name] = std::move(this->value_stack.back());
                this->value_stack.pop_back();
            } catch (std::out_of_range& err) {
                throw pyerror("op::STORE_FAST tried to store name out of range");
            }
            GOTO_NEXT_OP;
        CASE(STORE_NAME)
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
            GOTO_NEXT_OP;
        }
        CASE(LOAD_CONST)
        {
            try {
                DEBUG("op::LOAD_CONST pushed constant at index %d", (int)arg);
                this->value_stack.push_back(
                    this->code->co_consts.at(arg)
                );
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_CONST tried to load constant out of range");
            }
            GOTO_NEXT_OP ;
        }
        CASE(CALL_FUNCTION)
        {
            DEBUG("op::CALL_FUNCTION attempted to call a function with %d arguments", arg);
            this->check_stack_size(1 + arg);
            
            // Instead of reading out into a vector, can I just pass have 'initialize_from_pyfunc'
            // read directly off the stack?
            ArgList args(
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

            CONTEXT_SWITCH ;
        }
        CASE(POP_TOP)
        {
            this->check_stack_size(1);
            this->value_stack.pop_back();
            GOTO_NEXT_OP ;
        }
        CASE(ROT_TWO)
        {
            this->check_stack_size(2);
            std::swap(*(this->value_stack.end()), *(this->value_stack.end() - 1));
            GOTO_NEXT_OP ;
        }
        CASE(COMPARE_OP)
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
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP;
        }
        CASE(INPLACE_ADD)
            this->check_stack_size(2);
        // see https://stackoverflow.com/questions/15376509/when-is-i-x-different-from-i-i-x-in-python
        // INPLACE_ADD should call __iadd__ method on full objects, falls back to __add__ if not available.
            if(attempt_inplace_op(*this,"__iadd__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_ADD)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::add_visitor(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP ;
        }
        CASE(INPLACE_SUBTRACT)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__isub__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_SUBTRACT)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_sub>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP ;
        }
        CASE(INPLACE_FLOOR_DIVIDE)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__ifloordiv__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_FLOOR_DIVIDE)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_divide>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP ;
        }
        CASE(INPLACE_MULTIPLY)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__imul__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_MULTIPLY)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_mult>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP ;
        }
        CASE(INPLACE_MODULO)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__imod__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_MODULO)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_modulo>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP ;
        }
        CASE(INPLACE_POWER)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__ipow__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_POWER)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_pow>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP ;
        }
        CASE(INPLACE_TRUE_DIVIDE)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__itruediv__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_TRUE_DIVIDE)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_true_div>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP ;
        }
        CASE(INPLACE_LSHIFT)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__ilshift__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_LSHIFT)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_lshift>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP ;
        }
        CASE(INPLACE_RSHIFT)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__irshift__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_RSHIFT)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_rshift>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP;
        }
        CASE(INPLACE_AND)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__iand__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_AND)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_and>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP;
        }
        CASE(INPLACE_XOR)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__ixor__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_XOR)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_xor>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP;
        }
        CASE(INPLACE_OR)
            this->check_stack_size(2);
            if(attempt_inplace_op(*this,"__ior__")){ 
                CONTEXT_SWITCH_IF_NEEDED;
                GOTO_NEXT_OP;
            }
        CASE(BINARY_OR)
        {
            this->check_stack_size(2);
            Value v2 = std::move(this->value_stack[this->value_stack.size() - 1]);
            Value v1 = std::move(this->value_stack[this->value_stack.size() - 2]);
            this->value_stack.resize(this->value_stack.size() - 2);
            std::visit(eval_helpers::numeric_visitor<eval_helpers::op_or>(*this),v1,v2);
            CONTEXT_SWITCH_IF_NEEDED;
            GOTO_NEXT_OP;
        }
        CASE(RETURN_VALUE)
        {
            this->check_stack_size(1);

            DEBUG("\tRETURNED FRAME'S FLAGS WERE: %x", (uint32_t)this->flags);

            if (this->get_flag(FrameState::FLAG_DONT_RETURN | FrameState::FLAG_CLASS_INIT_FRAME | FrameState::FLAG_OBJECT_INIT_FRAME)) {
                DEBUG_ADV("POPPED FRAME, NO RETURN");
                this->interpreter_state->pop_frame();
                return;
            }

            if (this->get_flag(FrameState::FLAG_IS_GENERATOR_FUNCTION)) {
                DEBUG_ADV("POPPED FRAME, IS GENERATOR");
                this->parent_frame->value_stack.push_back(false);
                this->interpreter_state->pop_frame();
                return;
            }

            DEBUG_ADV("REGULAR OLD RETURN");

            if (this->parent_frame != nullptr) {
                this->parent_frame->value_stack.push_back(std::move(this->value_stack.back()));
            }

            this->interpreter_state->pop_frame();
            return ;
        }
        CASE(SETUP_LOOP)
        {
            Block newBlock;
            newBlock.type = Block::Type::LOOP;
            newBlock.level = this->value_stack.size();
            newBlock.pc_start = this->code->instructions[this->r_pc + 1].bytecode_index; // GARETH: changed this offset to 1
            newBlock.pc_delta = arg;
            this->block_stack.push(newBlock);
            DEBUG("new block stack height: %lu", this->block_stack.size())
            GOTO_NEXT_OP;
        }
        CASE(BREAK_LOOP)
        {
            Block topBlock = this->block_stack.top();
            this->block_stack.pop();
            size_t jumpTo = this->code->pc_map[(topBlock.pc_start + topBlock.pc_delta)];
            if (jumpTo == 0) {
                throw pyerror("invalid jump destination for break loop!");
            }

            this->r_pc = jumpTo;
            GOTO_TARGET_OP;
        }
        CASE(POP_BLOCK)
            this->block_stack.pop();
            GOTO_NEXT_OP;
        CASE(POP_JUMP_IF_FALSE)
        {
            this->check_stack_size(1);
            // TODO: implement handling for truthy values.
            Value top = std::move(this->value_stack.back());
            this->value_stack.pop_back();

            if (!std::visit(value_helper::visitor_is_truthy(), top)) {
                this->r_pc = arg;
                GOTO_TARGET_OP;
            }
            GOTO_NEXT_OP;
        }
        CASE(JUMP_ABSOLUTE)
            this->r_pc = arg;
            GOTO_TARGET_OP;
        CASE(JUMP_FORWARD)
            this->r_pc += arg;
            GOTO_TARGET_OP;
        CASE(MAKE_CLOSURE)
        {
            // Loooots of copy/paste here
            // I really should factor the copied part of make_closure/make_function into a function,
            // But since I will just undo that later for direct threading i guess it's copy/paste time
            this->check_stack_size(arg + 3);

            // Pop the name and code
            Value name = std::move(value_stack.back());
            this->value_stack.pop_back();
            Value closure_code = std::move(value_stack.back());
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
                    value::PyFunc {std::get<ValueString>(name), std::get<ValueCode>(closure_code), v}
                );
                // CHange to a tuple!
                nv->__closure__ = closure;
                this->value_stack.push_back(nv);
            } catch (std::bad_variant_access&) {
                std::stringstream ss;
                ss << "MAKE FUNCTION called with name '" << name << "' and code block: " << closure_code;
                ss << ", but make function expects string and code object";
                throw pyerror(ss.str());
            }
            GOTO_NEXT_OP;
        }
        CASE(MAKE_FUNCTION)
        {

            // DONT FORGET TO CHANGE MAKE_CLOSURE TOO WHEN YOU CHANGE HOW ARG IS USED HERE

            this->check_stack_size(arg + 2);

            // Pop the name and code
            ValueString name;
            ValueCode func_code;
            try {
                name = std::move(std::get<ValueString>(value_stack.back()));
                this->value_stack.pop_back();
                func_code = std::move(std::get<ValueCode>(value_stack.back()));
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
                    value::PyFunc {name, func_code, v}
                )
            );
            GOTO_NEXT_OP;
        }
        CASE(LOAD_BUILD_CLASS)
        {
            // Push the build class builtin onto the stack
            J_DEBUG("Preparing to build a class");
            this->value_stack.push_back((*(this->interpreter_state->ns_builtins))["__build_class__"]);
            GOTO_NEXT_OP;
        }
        CASE(LOAD_ATTR)
        {
            this->check_stack_size(1);
            DEBUG("Loading Attr %s",this->code->co_names[arg].c_str()) ;
            Value val = std::move(value_stack.back());
            this->value_stack.pop_back();
            // Visit a load_attr_visitor constructed with the frame state and the arg to get
            // Do it this way because val might turn out to be a PyClass or a PyObject
            std::visit(
                value_helper::load_attr_visitor(*this,this->code->co_names[arg]),
                val
            );
            GOTO_NEXT_OP;
        }
        CASE(STORE_ATTR)
        {
            this->check_stack_size(2);
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
                GOTO_NEXT_OP;
            }
            auto vpc = std::get_if<ValuePyClass>(&tos);
            if(vpc != NULL){
                (*vpc)->store_attr(this->code->co_names[arg],val);
                GOTO_NEXT_OP;
            }

            // We should never get here
            throw pyerror(std::string("STORE_ATTR called with bad stack!"));
            GOTO_NEXT_OP;
        }
        CASE(BUILD_TUPLE)
        CASE(BUILD_LIST)
        {
            this->check_stack_size(arg);

            // Pop the arguments to turn into a list.
            ValueList newList = this->interpreter_state->alloc.heap_lists.make();

            newList->values.assign(this->value_stack.end() - arg, this->value_stack.end());
            this->value_stack.resize(this->value_stack.size() - arg);
            
            this->value_stack.push_back(newList);

            GOTO_NEXT_OP;
        }
        CASE(BINARY_SUBSCR)
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

            GOTO_NEXT_OP ;
        }
        CASE(GET_ITER)
        {
            DEBUG_ADV("GET_ITER IS A NULL OP FOR NOW, WHEN LIST ITERATION IS IMPLEMENTED IT WILL WORK");
            GOTO_NEXT_OP ;
        }
        CASE(FOR_ITER)
        {
            DEBUG_ADV("\tJUMP OFFSET FOR ITERATOR: " << arg);
            std::visit(eval_helpers::for_iter_visitor {*this, arg}, this->value_stack.back());
            CONTEXT_SWITCH_KEEP_PC;
        }
        CASE(YIELD_VALUE)
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

                CONTEXT_SWITCH;
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

                CONTEXT_SWITCH_KEEP_PC; // return so that this op will be run again.
            }
        }

        CASE(STORE_SUBSCR)
        {
            this->check_stack_size(3);
            Value key = std::move(this->value_stack.back());
            this->value_stack.pop_back();
            Value self = std::move(this->value_stack.back());
            this->value_stack.pop_back();
            Value value = std::move(this->value_stack.back());
            this->value_stack.pop_back();

            std::visit(eval_helpers::store_subscr_visitor {key, value}, self);
            GOTO_NEXT_OP ;
        }
        
        CASE(ROT_THREE)
        CASE(DUP_TOP)
        CASE(DUP_TOP_TWO)
        CASE(UNARY_POSITIVE)
        CASE(UNARY_NEGATIVE)
        CASE(UNARY_NOT)
        CASE(UNARY_INVERT)
        CASE(BINARY_MATRIX_MULTIPLY)
        CASE(INPLACE_MATRIX_MULTIPLY)
        CASE(GET_AITER)
        CASE(GET_ANEXT)
        CASE(BEFORE_ASYNC_WITH)
        CASE(DELETE_SUBSCR)
        CASE(GET_YIELD_FROM_ITER)
        CASE(PRINT_EXPR)
        CASE(YIELD_FROM)
        CASE(GET_AWAITABLE)
        CASE(WITH_CLEANUP_START)
        CASE(WITH_CLEANUP_FINISH)
        CASE(IMPORT_STAR)
        CASE(SETUP_ANNOTATIONS)
        CASE(END_FINALLY)
        CASE(POP_EXCEPT)
        CASE(DELETE_NAME)
        CASE(UNPACK_SEQUENCE)
        CASE(UNPACK_EX)
        CASE(DELETE_ATTR)
        CASE(DELETE_GLOBAL)
        CASE(BUILD_SET)
        CASE(BUILD_MAP)
        CASE(IMPORT_NAME)
        CASE(IMPORT_FROM)
        CASE(JUMP_IF_FALSE_OR_POP)
        CASE(JUMP_IF_TRUE_OR_POP)
        CASE(POP_JUMP_IF_TRUE)
        CASE(CONTINUE_LOOP)
        CASE(SETUP_EXCEPT)
        CASE(SETUP_FINALLY)
        CASE(DELETE_FAST)
        CASE(STORE_ANNOTATION)
        CASE(RAISE_VARARGS)
        CASE(BUILD_SLICE)
        CASE(DELETE_DEREF)
        CASE(CALL_FUNCTION_KW)
        CASE(CALL_FUNCTION_EX)
        CASE(SETUP_WITH)
        CASE(EXTENDED_ARG)
        CASE(LIST_APPEND)
        CASE(SET_ADD)
        CASE(MAP_ADD)
        CASE(BUILD_LIST_UNPACK)
        CASE(BUILD_MAP_UNPACK)
        CASE(BUILD_MAP_UNPACK_WITH_CALL)
        CASE(BUILD_TUPLE_UNPACK)
        CASE(BUILD_SET_UNPACK)
        CASE(SETUP_ASYNC_WITH)
        CASE(FORMAT_VALUE)
        CASE(BUILD_CONST_KEY_MAP)
        CASE(BUILD_STRING)
        CASE(BUILD_TUPLE_UNPACK_WITH_CALL)
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