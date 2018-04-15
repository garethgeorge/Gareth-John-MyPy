#include <stdio.h>
#include <iostream>
#include <utility>
#include <boost/variant/get.hpp>

#include "pyframe.hpp"
#include "pyinterpreter.hpp"
#include "../lib/oplist.hpp"
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
    
    struct add_visitor: public boost::static_visitor<Value> {
        Value operator()(double v1, double v2) const {
            return v1 + v2;
        }
        Value operator()(double v1, int64_t v2) const {
            return v1 + v2;
        }
        Value operator()(int64_t v1, double v2) const {
            return v1 + v2;
        }
        Value operator()(int64_t v1, int64_t v2) const {
            return v1 + v2;
        }
        
        Value operator()(const std::shared_ptr<std::string>& v1, const std::shared_ptr<std::string> &v2) const {
            return std::make_shared<std::string>(*v1 + *v2);
        }

        template<typename T1, typename T2>
        Value operator()(T1, T2) const {
            throw pyerror(string("type error in add, can not add values of types ") + typeid(T1).name() + " and " + typeid(T2).name());
        }
    };
    
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

    struct op_sub {
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 - v2;
        }
    };

    struct op_mult {
        template<typename T1, typename T2>
        static auto action(T1 v1, T2 v2) {
            return v1 * v2;
        }
    };
    
    template<class T>
    struct numeric_visitor: public boost::static_visitor<Value> {
        Value operator()(double v1, double v2) const {
            return T::action(v1, v2);
        }
        Value operator()(double v1, int64_t v2) const {
            return T::action(v1, v2);
        }
        Value operator()(int64_t v1, double v2) const {
            return T::action(v1, v2);
        }
        Value operator()(int64_t v1, int64_t v2) const {
            return T::action(v1, v2);
        }
        
        template<typename T1, typename T2>
        Value operator()(T1, T2) const {
            throw pyerror(string("type error in numeric_visitor, can not work on values of types ") + typeid(T1).name() + " and " + typeid(T2).name());
        }
    };
    
    struct call_visitor: public boost::static_visitor<void> {
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


void FrameState::eval_next() {
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
                const size_t cache_idx = arg % (sizeof(this->ns_local_shortcut) / sizeof(Value *));
                if (this->ns_local_shortcut[cache_idx].value != nullptr &&
                    this->ns_local_shortcut[cache_idx].key == &name) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a local from ns_local_shortcut", name.c_str());
                    this->value_stack.push(*(this->ns_local_shortcut[cache_idx].value));
                } else {
#endif
                    const auto& globals = this->interpreter_state->ns_globals;
                    const auto& builtins = this->interpreter_state->ns_bulitins;
                    auto itr_local = this->ns_local.find(name);
                    if (itr_local != this->ns_local.end()) {
                        DEBUG("op::LOAD_NAME ('%s') loaded a local", name.c_str());
                        this->value_stack.push(itr_local->second);
#ifdef OPT_FRAME_NS_LOCAL_SHORTCUT
                        this->ns_local_shortcut[cache_idx].key = &name;
                        this->ns_local_shortcut[cache_idx].value = &(itr_local->second);
#endif
                    } else {
                        auto itr_global = globals.find(name);
                        if (itr_global != globals.end()) {
                            DEBUG("op::LOAD_NAME ('%s') loaded a global", name.c_str());
                            this->value_stack.push(itr_global->second);
                        } else {
                            auto itr_builtin = builtins.find(name);
                            if (itr_builtin != builtins.end()) {
                                DEBUG("op::LOAD_NAME ('%s') loaded a builtin", name.c_str());
                                this->value_stack.push(itr_builtin->second);
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
                this->ns_local[name] = this->value_stack.top();
                this->value_stack.pop();
            } catch (std::out_of_range& err) {
                throw pyerror("op::LOAD_NAME tried to store name out of range");
            }
            break;
        }
        case op::LOAD_CONST:
        {
            try {
                DEBUG("op::LOAD_CONST pushed constant at index %d", (int)arg);
                this->value_stack.push(
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
            std::vector<Value> args;
            args.reserve(arg);
            for (int i = 0; i < arg; ++i) {
                args.push_back(this->value_stack.top());
                this->value_stack.pop();
            }
            boost::apply_visitor(eval_helpers::call_visitor(*this, args), this->value_stack.top());
            this->value_stack.pop();
            break ;
        }
        case op::POP_TOP:
        {
            this->value_stack.pop();
            break ;
        }
        case op::ROT_TWO:
        {
            auto temp1 = this->value_stack.top();
            this->value_stack.pop();
            auto temp2 = this->value_stack.top();
            this->value_stack.pop();
            this->value_stack.push(temp1);
            this->value_stack.push(temp2);
            break ;
        }
        case op::COMPARE_OP:
        {
            const Value val2 = this->value_stack.top();
            this->value_stack.pop();
            const Value val1 = this->value_stack.top();
            this->value_stack.pop();
            Value result;
            switch (arg) {
                case op::cmp::LT:
                    result = boost::apply_visitor(
                        eval_helpers::numeric_visitor<eval_helpers::op_lt>(),
                        val1, val2);
                    break;
                case op::cmp::LTE:
                    result = boost::apply_visitor(
                        eval_helpers::numeric_visitor<eval_helpers::op_lte>(),
                        val1, val2);
                    break;
                case op::cmp::GT:
                    result = boost::apply_visitor(
                        eval_helpers::numeric_visitor<eval_helpers::op_gt>(),
                        val1, val2);
                    break;
                case op::cmp::GTE:
                    result = boost::apply_visitor(
                        eval_helpers::numeric_visitor<eval_helpers::op_gte>(),
                        val1, val2);
                    break;
                default:
                    throw pyerror(string("operator ") + op::cmp::name[arg] + " not implemented.");
            }
            this->value_stack.push(result);
            break;
        }
        case op::INPLACE_ADD:
        // see https://stackoverflow.com/questions/15376509/when-is-i-x-different-from-i-i-x-in-python
        // INPLACE_ADD should call __iadd__ method on full objects, falls back to __add__ if not available.
        case op::BINARY_ADD:
        {
            const Value val2 = this->value_stack.top();
            this->value_stack.pop();
            const Value val1 = this->value_stack.top();
            this->value_stack.pop();
            this->value_stack.push(boost::apply_visitor(eval_helpers::add_visitor(), val1, val2));
            break ;
        }
        case op::BINARY_SUBTRACT:
        {
            const Value val2 = this->value_stack.top();
            this->value_stack.pop();
            const Value val1 = this->value_stack.top();
            this->value_stack.pop();
            this->value_stack.push(boost::apply_visitor(
                eval_helpers::numeric_visitor<eval_helpers::op_sub>(), 
                val1, val2
            ));
            break;
        }
        case op::BINARY_MULTIPLY:
        {
            const Value val2 = this->value_stack.top();
            this->value_stack.pop();
            const Value val1 = this->value_stack.top();
            this->value_stack.pop();
            this->value_stack.push(boost::apply_visitor(
                eval_helpers::numeric_visitor<eval_helpers::op_mult>(), 
                val1, val2
            ));
            break;
        }
        case op::RETURN_VALUE:
        {
            auto val = this->value_stack.top();
            if (this->parent_frame != nullptr) {
                this->parent_frame->value_stack.push(val);
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
            DEBUG("new block stack height: %llu", this->block_stack.size())
            break ;
        }
        case op::POP_BLOCK:
            this->block_stack.pop();
            break ;
        case op::POP_JUMP_IF_FALSE:
        {
            // TODO: implement handling for truthy values.
            auto top = this->value_stack.top();
            this->value_stack.pop();
            try {
                if (!boost::get<bool>(top)) {
                    this->r_pc = arg;
                }
            } catch (boost::bad_get& e) {
                // TODO: does not actually matter what the type of the condition is :o
                throw pyerror("expected condition to have type bool, got bad type.");
            }
            break;
        }
        case op::JUMP_ABSOLUTE:
            this->r_pc = arg;
            return ;
        default:
        {
            DEBUG("UNIMPLEMENTED BYTECODE: %s", op::name[bytecode])
            throw pyerror("UNIMPLEMENTED BYTECODE");
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

}