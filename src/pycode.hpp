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

    uint64_t co_stacksize;
    uint64_t co_nlocals;
    std::vector<ByteCode> bytecode;
    std::vector<Value> co_consts;
    std::vector<std::string> co_names;
    std::vector<std::string> co_varnames;
    
    Code(const json& tree);
    ~Code();
    
    static std::shared_ptr<Code> from_program(const std::string& python, const std::string& compilePyPath);
};

}

#endif