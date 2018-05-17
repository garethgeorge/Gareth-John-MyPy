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

    std::string co_name;
    uint64_t co_stacksize;
    uint64_t co_nlocals;
    uint64_t co_argcount;
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
    std::vector<LineNoMapping> lnotab; // lookup the line number that the error happened on
    
    Code(const json& tree);
    ~Code();
    
    static std::shared_ptr<Code> from_program(const std::string& python, const std::string& compilePyPath);

    void print_bytecode() const;
};

}

#endif