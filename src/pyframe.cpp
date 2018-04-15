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
#define DEBUG_ON
#include "../lib/debug.hpp"

using std::string;

namespace py {

FrameState::FrameState(
        InterpreterState *interpreter_state, 
        FrameState *parent_frame, 
        std::shared_ptr<Code>& code) 
{
    this->interpreter_state = interpreter_state;
    this->parent_frame = parent_frame;
    this->code = code;
}

void FrameState::print_next() {
    Code::ByteCode bytecode = code->bytecode[this->r_pc];
    if (this->r_pc >= this->code->bytecode.size()) {
        printf("popped a frame from the call stack dealio.");
        this->interpreter_state->callstack.pop();
        return ;
    }

    if (bytecode == 0) {
        this->r_pc++;
        return ;
    }

    printf("%10llu %s\n", this->r_pc, op::name[bytecode]);
    if (bytecode == op::LOAD_CONST) {
        // printf("\tconstant: %s\n", ((std::string)code->constants[code->bytecode[this->r_pc + 1]]).c_str());
    }
    
    if (bytecode < op::HAVE_ARGUMENT) {
        this->r_pc += 1;
    } else {
        this->r_pc += 2;
    }
}

namespace eval_helpers {

    using value_helper::numeric_visitor;
    
    struct op_lt {
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 < v2;
        }
    };

    struct op_lte {
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 <= v2;
        }
    };

    struct op_gt {
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 > v2;
        }
    };

    struct op_gte {
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 >= v2;
        }
    };

    struct op_eq {
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 == v2;
        }
    };

    struct op_neq {
        template<typename T1, typename T2>
        static bool action(T1 v1, T2 v2) {
            return v1 != v2;
        }
    };

    struct op_sub {
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 - v2;
        }
    };

    struct op_add {
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 + v2;
        }
    };

    struct op_mult {
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 * v2;
        }
    };

    struct op_divide {
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 / v2;
        }
    };

    struct op_modulo {
        static auto action(int64_t v1, int64_t v2) {
            return v1 % v2;
        }

        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return std::fmod(v1, v2);
        }
    };

    struct add_visitor: public numeric_visitor<op_add> {
        using numeric_visitor<op_add>::operator();
        
        Value operator()(const std::shared_ptr<std::string>& v1, const std::shared_ptr<std::string> &v2) const {
            return std::make_shared<std::string>(*v1 + *v2);
        }
    };
    
    struct call_visitor {
        FrameState& frame;
        std::vector<Value>& args;
        call_visitor(FrameState& frame, std::vector<Value>& args) : frame(frame), args(args) {}

        void operator()(const std::shared_ptr<const value::CFunction>& func) const {
            DEBUG("call_visitor dispatching CFunction->action");
            func->action(frame, args);
        }
        
        template<typename T>
        void operator()(T) const {
            throw pyerror(string("can not call object of type ") + typeid(T).name());
        }
    };
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
                auto& local_shortcut_entry = this->ns_local_shortcut[arg % (sizeof(this->ns_local_shortcut) / sizeof(Value *))];
                if (local_shortcut_entry.value != nullptr &&
                    local_shortcut_entry.key == &name) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a local from ns_local_shortcut", name.c_str());
                    this->value_stack.push_back(*(local_shortcut_entry.value));
                } else {
#endif
                    const auto& globals = this->interpreter_state->ns_globals;
                    const auto& builtins = this->interpreter_state->ns_bulitins;
                    auto itr_local = this->ns_local.find(name);
                    if (itr_local != this->ns_local.end()) {
                        DEBUG("op::LOAD_NAME ('%s') loaded a local", name.c_str());
                        this->value_stack.push_back(itr_local->second);
#ifdef OPT_FRAME_NS_LOCAL_SHORTCUT
                        local_shortcut_entry.key = &name;
                        local_shortcut_entry.value = &(itr_local->second);
#endif
                    } else {
                        auto itr_global = globals.find(name);
                        if (itr_global != globals.end()) {
                            DEBUG("op::LOAD_NAME ('%s') loaded a global", name.c_str());
                            this->value_stack.push_back(itr_global->second);
                        } else {
                            auto itr_builtin = builtins.find(name);
                            if (itr_builtin != builtins.end()) {
                                DEBUG("op::LOAD_NAME ('%s') loaded a builtin", name.c_str());
                                this->value_stack.push_back(itr_builtin->second);
                            } else {
                                throw pyerror(string("op::LOAD_NAME name not found: ") + name);
                            }
                        }
                    }
#ifdef OPT_FRAME_NS_LOCAL_SHORTCUT
                }
#endif
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_NAME tried to load name out of range");
            }
            break ;
        }
        case op::STORE_NAME:
        {
            try {
                const std::string& name = this->code->co_names.at(arg);
                auto& local_shortcut_entry = this->ns_local_shortcut[arg % (sizeof(this->ns_local_shortcut) / sizeof(Value *))];
                if (local_shortcut_entry.value != nullptr &&
                    local_shortcut_entry.key == &name) {
                    *(local_shortcut_entry.value) = std::move(this->value_stack.back());
                } else {
                    this->ns_local[name] = std::move(this->value_stack.back());
                }
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
            // TODO: optimize function call to use std::move instead!!!
            // or better, have functions take a begin and end range iterator
            std::vector<Value> args(this->value_stack.end() - arg, this->value_stack.end());
            this->value_stack.resize(this->value_stack.size() - arg);
            std::visit(eval_helpers::call_visitor(*this, args), this->value_stack.back());
            this->value_stack.pop_back();
            break ;
        }
        case op::POP_TOP:
        {
            this->value_stack.pop_back();
            break ;
        }
        case op::ROT_TWO:
        {
            // TODO: use std::swap here
            std::swap(*(this->value_stack.end()), *(this->value_stack.end() - 1));
            break ;
        }
        case op::COMPARE_OP:
        {
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
                        val1, val2
                    );
                    break ;
                case op::cmp::NEQ:
                    result = std::visit(
                        eval_helpers::numeric_visitor<eval_helpers::op_neq>(),
                        val1, val2
                    );
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
            this->value_stack[this->value_stack.size() - 2] = 
                std::move(std::visit(eval_helpers::numeric_visitor<eval_helpers::op_modulo>(),
                    this->value_stack[this->value_stack.size() - 2],
                    this->value_stack[this->value_stack.size() - 1]));
            this->value_stack.pop_back();
            break ;
        }
        case op::RETURN_VALUE:
        {
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
            // TODO: implement handling for truthy values.
            Value top = std::move(this->value_stack.back());
            this->value_stack.pop_back();
            try {
                if (!std::get<bool>(top)) {
                    this->r_pc = arg;
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
        default:
        {
            DEBUG("UNIMPLEMENTED BYTECODE: %s", op::name[bytecode])
            throw pyerror(string("UNIMPLEMENTED BYTECODE ") + op::name[bytecode])   ;
        }
    }
    
    // REMINDER: some instructions like op::JUMP_ABSOLUTE return early, so they
    // do not reach this point
    
    if (bytecode < op::HAVE_ARGUMENT) {
        this->r_pc += 1;
    } else {
        this->r_pc += 2;
    }
}

void InterpreterState::eval() {
    while (!this->callstack.empty()) {
        // TODO: try caching the top of the stack
        #ifdef PRINT_OPS
        this->callstack.top().eval_print();
        #else
        this->callstack.top().eval_next();
        #endif
    }
}

}