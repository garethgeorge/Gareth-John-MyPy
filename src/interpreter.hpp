#ifndef STACKS_H
#define STACKS_H

#include <stdint.h>
#include <vector>
#include <stack>

namespace mypy {

struct Value {
    // TODO: actually make this a virtual class and have
    // literal types inherit from it ?
    // Don't forget to mark destructor virtual
    uint8_t type;
    uint64_t value;
};


struct Frame {
    Frame *parent_frame = nullptr;
    std::stack<Block *> block_stack;
    std::stack<Value> value_stack;
}

}


#endif STACKS_H