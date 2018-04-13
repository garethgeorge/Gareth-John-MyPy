#include <stdio.h>

#include "pyinterpreter.hpp"
#include "../lib/oplist.hpp"

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

void FrameState::eval_next() {
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

}