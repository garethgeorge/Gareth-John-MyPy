#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdint.h>
#include <vector>
#include <stack>
#include <unordered_map>
#include <stdexcept>
#include <boost/property_tree/ptree_fwd.hpp>

#include "pyerror.hpp"
#include "pyvalue.hpp"

namespace py {

struct Code;
struct InterpreterState;

// see https://www.boost.org/doc/libs/1_55_0/doc/html/variant/tutorial.html
// we use boost::variant as a compact union type carrying type information

using Namespace = std::unordered_map<std::string, Value>;

struct Code {
    using ByteCode = uint8_t;

    std::vector<ByteCode> bytecode;
    std::vector<Value> co_consts;
    std::vector<std::string> co_names;
    
    Code(const boost::property_tree::ptree& tree);
    ~Code();
};

struct Block {
    enum Type {
        NONE,
        LOOP,
        EXCEPT,
        FINALLY
    };
    Type type = NONE;
    size_t pc_start = 0;
    size_t pc_delta = 0;
    size_t level = 0; // value level to pop up to
};

struct FrameState {
    uint64_t r_pc = 0; // program counter
    FrameState *parent_frame = nullptr;
    InterpreterState *interpreter_state = nullptr;
    std::shared_ptr<Code> code;
    std::stack<Value> value_stack;
    std::stack<Block> block_stack;
    Namespace ns_local;
    Value *ns_local_shortcut[256] = {0};
    
    FrameState(
        InterpreterState *interpreter_state, 
        FrameState *parent_frame, 
        std::shared_ptr<Code>& code);

    void eval_next();
    void print_next();
};

struct InterpreterState {
    std::stack<FrameState> callstack;
    Namespace ns_globals;
    Namespace ns_bulitins;

    InterpreterState(std::shared_ptr<Code>& code);

    void eval();
};

}


#endif 