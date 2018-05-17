#pragma once
#ifndef PYCODE_H
#define PYCODE_H

#include <memory>
#include <vector>
#include <string>

#include "pyvalue.hpp"
#include "../lib/json_fwd.hpp"

using json = nlohmann::json;

namespace py {

struct Code {
    using ByteCode = uint8_t;

    static constexpr const uint8_t FLAG_IS_GENERATOR_FUNCTION = 1; // contains the opcode op::YIELD_VALUE;

    uint8_t flags = 0;
    std::string co_name;
    uint64_t co_stacksize;
    uint64_t co_nlocals;
    uint64_t co_argcount;
    std::vector<uint64_t> pc_map;
    std::vector<ByteCode> bytecode;
    std::vector<Value> co_consts;
    std::vector<std::string> co_names;
    std::vector<std::string> co_varnames;
    std::vector<std::string> co_cellvars;
    std::vector<std::string> co_freevars;

    struct LineNoMapping {
        uint64_t line;
        uint64_t pc;
    };

    struct Instruction {
        ByteCode bytecode;
        size_t bytecode_index; // position in the bytecode table
        uint64_t arg;
    };

    std::vector<LineNoMapping> lnotab; // lookup the line number that the error happened on
    std::vector<Instruction> instructions; // we decode instructions at this step to make later analysis easier
    
    Code(const json& tree);
    ~Code();
    
    static std::shared_ptr<Code> from_program(const std::string& python, const std::string& compilePyPath);

    // flag getters and setters
    inline bool get_flag(const uint8_t flag) const {
        return this->flags & flag;
    }

    inline void set_flag(const uint8_t flag) {
        this->flags |= flag;
    }

    inline void clear_flag(const uint8_t flag) {
        this->flags &= (~flag);
    }
};


}

#endif