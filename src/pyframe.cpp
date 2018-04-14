#include <stdio.h>
#include <iostream>

#include "pyinterpreter.hpp"
#include "../lib/oplist.hpp"
#include "pyvalue.hpp"
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

    struct typename_visitor: public boost::static_visitor<std::string> {
        std::string operator()(double) const {
            return "double";
        }

        std::string operator()(int64_t) const {
            return "int64_t";
        }

        std::string operator()(std::shared_ptr<std::string> str) const {
            return std::string("(string)") + *str;
        }

        std::string operator()(std::shared_ptr<const Code>) const {
            return "code";
        }

        std::string operator()(std::shared_ptr<const value::CFunction>) const {
            return "CFunction";
        }

        std::string operator()(value::NoneType) const {
            return "NoneType";
        }
    };
    
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
    uint8_t arg = code->bytecode[this->r_pc + 1];

    if (this->r_pc >= this->code->bytecode.size()) {
        DEBUG("overflowed program, popped stack frame, however this indicates a failure so we will exit.");
        this->interpreter_state->callstack.pop();
        exit(0);
        return ;
    }
    
    DEBUG("EVALUATE BYTECODE: %s", op::name[bytecode])
    switch (bytecode) {
        case 0:
            break;
        case op::LOAD_NAME:
        {
            try {
                const std::string& name = this->code->co_names.at(arg);
                auto globals = this->interpreter_state->ns_globals;
                auto builtins = this->interpreter_state->ns_bulitins;

                auto itr_local = this->ns_local.find(name);
                if (itr_local != this->ns_local.end()) {
                    DEBUG("op::LOAD_NAME ('%s') loaded a local", name.c_str());
                    this->value_stack.push(itr_local->second);
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

            break ;
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
        case op::BINARY_ADD:
        {
            const Value val2 = this->value_stack.top();
            this->value_stack.pop();
            const Value val1 = this->value_stack.top();
            this->value_stack.pop();
            const Value result = boost::apply_visitor(eval_helpers::add_visitor(), val1, val2);
            this->value_stack.push(result);
            break ;
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
        default:
        {
            DEBUG("UNIMPLEMENTED BYTECODE: %s", op::name[bytecode])
        }
    }
    
    if (bytecode < op::HAVE_ARGUMENT) {
        this->r_pc += 1;
    } else {
        this->r_pc += 2;
    }
}

}