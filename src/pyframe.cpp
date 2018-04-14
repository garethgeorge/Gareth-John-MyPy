#include <stdio.h>

#include "pyinterpreter.hpp"
#include "../lib/oplist.hpp"
#include "pyvalue_types.hpp"
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

void FrameState::eval_next() {
    Code::ByteCode bytecode = code->bytecode[this->r_pc];
    uint8_t arg = code->bytecode[this->r_pc + 1];

    if (this->r_pc >= this->code->bytecode.size()) {
        DEBUG("overflowed program, popped stack frame, however this indicates a failure so we will exit.");
        this->interpreter_state->callstack.pop();
        exit(0);
        return ;
    }
    
    switch (bytecode) {
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
                throw pyerror("op::LOAD_NAME tried to load constant out of range");
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