#pragma once
#ifndef PYCODE_H
#define PYCODE_H

#include <memory>
#include <vector>
#include <string>

#include "pyvalue.hpp"
#include "../lib/json_fwd.hpp"

namespace py {

using json = nlohmann::json;

struct Code {
    using ByteCode = uint8_t;

    std::vector<ByteCode> bytecode;
    std::vector<Value> co_consts;
    std::vector<std::string> co_names;
    
    Code(const json& tree);
    ~Code();

    static std::shared_ptr<Code> fromProgram(const std::string& python, const std::string& compilePyPath);
};

}

#endif