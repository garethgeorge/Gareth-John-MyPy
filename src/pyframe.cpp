#include <stdio.h>
#include <iostream>
#include <utility>
#include <algorithm>
#include <variant>
#include <cmath>
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
        std::shared_ptr<Code>& code) 
{
    DEBUG("constructed a new frame");
    this->interpreter_state = interpreter_state;
    this->parent_frame = parent_frame;
    this->code = code;
    DEBUG("reserved %lu bytes for the stack", code->co_stacksize);
    this->value_stack.reserve(code->co_stacksize);
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
    };

    struct op_lte { // a <= b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 <= v2;
        }
    };

    struct op_gt { // a > b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 > v2;
        }
    };

    struct op_gte { // a >= b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 >= v2;
        }
    };

    struct op_eq { // a == b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 == v2;
        }
    };

    struct op_neq { // a != b
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 != v2;
        }
    };

    struct op_sub { // a - b
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 - v2;
        }
    };

    struct op_add { // a + b
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 + v2;
        }
    };

    struct op_mult { // a * b
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 * v2;
        }
    };

    struct op_divide { // a / b
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 / v2;
        }
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
    };
    
    struct add_visitor: public numeric_visitor<op_add> {
        /*
            the add visitor is simply a numeric_visitor with op_add but also
            including one additional function for string addition
        */
        using numeric_visitor<op_add>::operator();
        
        Value operator()(const std::shared_ptr<std::string>& v1, const std::shared_ptr<std::string> &v2) const {
            return std::make_shared<std::string>(*v1 + *v2);
        }
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
        
        template<typename T>
        void operator()(T) const {
            throw pyerror(string("can not call object of type ") + typeid(T).name());
        }
    };
}

// Bad ugly copy paste but I got annoted at type errors
// Will improve later
void FrameState::print_value(Value& val) const {
    std::visit(value_helper::overloaded {
            [](auto&& arg) { throw pyerror("unimplemented stack printer for stack value"); },
            [](double arg) { std::cerr << "double(" << arg << ")"; },
            [](int64_t arg) { std::cerr << "int64(" << arg << ")"; },
            [](const ValueString arg) {std::cerr << "ValueString(" << *arg << ")"; },
            [](const ValueCFunction arg) {std::cerr << "CFunction()"; },
            [](const ValueCode arg) {std::cerr << "Code()"; },
            [](const ValuePyFunction arg) {std::cerr << "Python Code()"; },
            [](value::NoneType) {std::cerr << "None"; },
            [](bool val) {if (val) std::cerr << "bool(true)"; else std::cout << "bool(false)"; }
        }, val);
}

void FrameState::print_stack() const {
    std::cerr << "[";
    for (size_t i = 0; i < this->value_stack.size(); ++i) {
        if (i != 0) {
            std::cerr << ", ";
        }
        const Value& val = this->value_stack[i];
        std::cerr << i << "_";
        std::visit(value_helper::overloaded {
            [](auto&& arg) { throw pyerror("unimplemented stack printer for stack value"); },
            [](double arg) { std::cerr << "double(" << arg << ")"; },
            [](int64_t arg) { std::cerr << "int64(" << arg << ")"; },
            [](const ValueString& arg) {std::cerr << "ValueString(" << *arg << ")"; },
            [](const ValueCFunction& arg) {std::cerr << "CFunction()"; },
            [](const ValueCode& arg) {std::cerr << "Code()"; },
            [](const ValuePyFunction& arg) {std::cerr << "Python Code()"; },
            [](value::NoneType) {std::cerr << "None"; },
            [](bool val) {if (val) std::cerr << "bool(true)"; else std::cout << "bool(false)"; }
        }, val);
    }
    std::cerr << "].len = " << this->value_stack.size();

    std::cerr << std::endl;
}


inline void FrameState::eval_next() {
    Code::ByteCode bytecode = code->bytecode[this->r_pc];
    // TODO: figure out how to load extended arguments
    uint32_t arg = code->bytecode[this->r_pc + 1];
    
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
        case op::LOAD_NAME:
        {
            try {
                const std::string& name = this->code->co_names.at(arg);
#ifdef OPT_FRAME_NS_LOCAL_SHORTCUT
                // if NS_LOCAL_SHORTCUT optimization is turned on, see if we can
                // find the variable in the cache
                auto& local_shortcut_entry = this->ns_local_shortcut[arg % (sizeof(this->ns_local_shortcut) / sizeof(Value *))];
                if (local_shortcut_entry.value != nullptr &&
                    local_shortcut_entry.key == &name) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a local from ns_local_shortcut", name.c_str());
                    this->value_stack.push_back(*(local_shortcut_entry.value));
                    break ;
                } 
#endif
                const auto& globals = this->interpreter_state->ns_globals;
                const auto& builtins = this->interpreter_state->ns_builtins;
                auto itr_local = this->ns_local.find(name);
                if (itr_local != this->ns_local.end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a local", name.c_str());
                    this->value_stack.push_back(itr_local->second);
#ifdef OPT_FRAME_NS_LOCAL_SHORTCUT
                    // if NS_LOCAL_SHORTCUT optimization is turned on,
                    // turn on this optimization in the cache
                    local_shortcut_entry.key = &name;
                    local_shortcut_entry.value = &(itr_local->second);
#endif
                    break ;
                } 
                auto itr_global = globals.find(name);
                if (itr_global != globals.end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a global", name.c_str());
                    this->value_stack.push_back(itr_global->second);
                    break ;
                } 
                auto itr_builtin = builtins.find(name);
                if (itr_builtin != builtins.end()) {
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
        case op::STORE_NAME:
        {
            this->check_stack_size(1);
            try {
                const std::string& name = this->code->co_names.at(arg);

#ifdef OPT_FRAME_NS_LOCAL_SHORTCUT
                auto& local_shortcut_entry = 
                    this->ns_local_shortcut[arg % (sizeof(this->ns_local_shortcut) / sizeof(Value *))];
                if (local_shortcut_entry.value != nullptr &&
                    local_shortcut_entry.key == &name) {
                    *(local_shortcut_entry.value) = std::move(this->value_stack.back());
                } else {
                    this->ns_local[name] = std::move(this->value_stack.back());
                }
#else 
                this->ns_local[name] = std::move(this->value_stack.back());
#endif
                this->value_stack.pop_back();
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_NAME tried to store name out of range");
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
                eval_helpers::call_visitor(*this, args),
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
            const Value val2 = std::move(this->value_stack.back());
            this->value_stack.pop_back();
            const Value val1 = std::move(this->value_stack.back());
            this->value_stack.pop_back();
            Value result;
            DEBUG("\tCOMPARISON OPERATOR: %s", op::cmp::name[arg]);
            switch (arg) {
                case op::cmp::LT:
                    result = std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_lt>(),
                        val1, val2);
                    break;
                case op::cmp::LTE:
                    result = std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_lte>(),
                        val1, val2);
                    break;
                case op::cmp::GT:
                    result = std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_gt>(),
                        val1, val2);
                    break;
                case op::cmp::GTE:
                    result = std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_gte>(),
                        val1, val2);
                    break;
                case op::cmp::EQ:
                    result = std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_eq>(),
                        val1, val2);
                    break ;
                case op::cmp::NEQ:
                    result = std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_neq>(),
                        val1, val2);
                    break ;
                default:
                    throw pyerror(string("operator ") + op::cmp::name[arg] + " not implemented.");
            }
            this->value_stack.push_back(std::move(result));
            break;
        }
        case op::INPLACE_ADD:
        // see https://stackoverflow.com/questions/15376509/when-is-i-x-different-from-i-i-x-in-python
        // INPLACE_ADD should call __iadd__ method on full objects, falls back to __add__ if not available.
        case op::BINARY_ADD:
        {
            this->check_stack_size(2);
            this->value_stack[this->value_stack.size() - 2] = 
                std::move(std::visit(eval_helpers::add_visitor(), 
                    this->value_stack[this->value_stack.size() - 2],
                    this->value_stack[this->value_stack.size() - 1]));
            this->value_stack.pop_back();
            break ;
        }
        case op::INPLACE_SUBTRACT:
        case op::BINARY_SUBTRACT:
        {
            this->check_stack_size(2);
            this->value_stack[this->value_stack.size() - 2] = 
                std::move(std::visit(eval_helpers::numeric_visitor<eval_helpers::op_sub>(),
                    this->value_stack[this->value_stack.size() - 2],
                    this->value_stack[this->value_stack.size() - 1]));
            this->value_stack.pop_back();
            break ;
        }
        case op::INPLACE_FLOOR_DIVIDE:
        case op::BINARY_FLOOR_DIVIDE:
        {
            this->check_stack_size(2);
            this->value_stack[this->value_stack.size() - 2] = 
                std::move(std::visit(eval_helpers::numeric_visitor<eval_helpers::op_divide>(),
                    this->value_stack[this->value_stack.size() - 2],
                    this->value_stack[this->value_stack.size() - 1]));
            this->value_stack.pop_back();
            break ;
        }
        case op::INPLACE_MULTIPLY:
        case op::BINARY_MULTIPLY:
        {
            this->check_stack_size(2);
            this->value_stack[this->value_stack.size() - 2] = 
                std::move(std::visit(eval_helpers::numeric_visitor<eval_helpers::op_mult>(),
                    this->value_stack[this->value_stack.size() - 2],
                    this->value_stack[this->value_stack.size() - 1]));
            this->value_stack.pop_back();
            break ;
        }
        case op::INPLACE_MODULO:
        case op::BINARY_MODULO:
        {
            this->check_stack_size(2);
            this->value_stack[this->value_stack.size() - 2] = 
                std::move(std::visit(eval_helpers::numeric_visitor<eval_helpers::op_modulo>(),
                    this->value_stack[this->value_stack.size() - 2],
                    this->value_stack[this->value_stack.size() - 1]));
            this->value_stack.pop_back();
            break ;
        }
        case op::RETURN_VALUE:
        {
            this->check_stack_size(1);
            auto val = this->value_stack.back();
            if (this->parent_frame != nullptr) {
                this->parent_frame->value_stack.push_back(std::move(val));
            }
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
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////       case op::JUMP_ABSOLUTE:
            this->r_pc = arg;
            return ;
        case op::JUMP_FORWARD:
            this->r_pc += arg;
            return ;
        case op::MAKE_FUNCTION:
        {
            this->check_stack_size(arg + 2);

            // Pop the name and code
            Value name = std::move(value_stack.back());
            this->value_stack.pop_back();
            Value code = std::move(value_stack.back());
            this->value_stack.pop_back();

            // Pop the arguments
            /*std::vector<Value> v;
            v.reserve(arg);
            for(int i = 0;i < arg;i++){
                v.push_back(std::move(value_stack.back()));
                value_stack.pop_back();
            }*/

            std::vector<Value> v( this->value_stack.end() - arg, this->value_stack.end());
            this->value_stack.resize(this->value_stack.size() - arg);

#ifdef JOHN_PRINTS_ON
            fprintf(stderr,"Creating a function that accepts %d default args:\n",arg);
            print_value(name);
            fprintf(stderr,"\nThose default args are:\n",arg);
            for(int i = 0;i < v.size();i++){
                print_value(v[i]);
                fprintf(stderr,"\n",arg);
            }
#endif
            // Create the function object
            // Error here if the wrong types
            try {
                ValuePyFunction nv = std::make_shared<value::PyFunc>(value::PyFunc(std::get<ValueString>(name), 
                                                          std::get<ValueCode>(code), 
                                                          v));
                this->value_stack.push_back(nv);
            } catch (std::bad_variant_access&) {
                pyerror("MAKE_FUNCTION called with bad stack");
            }
            break;
        }
        default:
        {
            DEBUG("UNIMPLEMENTED BYTECODE: %s", op::name[bytecode])
            throw pyerror(string("UNIMPLEMENTED BYTECODE ") + op::name[bytecode])   ;
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
        this->r_pc += 2;
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
        std::cout << "ENCOUNTERED ERROR WHILE EVALUATING OPERATION: " 
            << op::name[frame.code->bytecode[frame.r_pc]] << std::endl;
        std::cout << "\tSTACK:";
        frame.print_stack();
        throw err;
    }
}

}