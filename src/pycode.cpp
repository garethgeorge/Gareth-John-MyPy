#include <iostream>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "pycode.hpp"
#include "pyvalue.hpp"
#include "../lib/base64.hpp"
// #define DEBUG_ON
#include "../lib/debug.hpp"
#include "../lib/json.hpp"

#define PROCXX_HAS_PIPE2 0 // not sure what this is, but it is not available on our build platform
#include "../lib/process.hpp"

namespace py {

Code::Code(const json& tree) {
    DEBUG("loading in source code from json");
    
#ifdef DEBUG_ON
    std::cout << __FILENAME__ << " ";
    std::cout << std::setw(4) << tree << std::endl;
#endif
    
    this->co_stacksize = tree.at("co_stacksize").get<uint64_t>();
    this->co_nlocals = tree.at("co_nlocals").get<uint64_t>();

    // decode the bytecode and push it into the bytecode property :)
    std::string base64bytecode = tree.at("co_code").get<std::string>();
    std::string bytecode = base64_decode(base64bytecode);
    this->bytecode.reserve(bytecode.length());
    
    for (const unsigned char c : bytecode) {
        this->bytecode.push_back(c);
    }

    // load constants
    const json& co_consts = tree.at("co_consts");
    DEBUG("loading in the co_consts constant pool");
    for (const json& element : co_consts) {
        const auto& type = element.at("type").get<std::string>();
        if (type == "literal") {
            const auto& real_type = element.at("real_type").get<std::string>();
            if (real_type == "<class 'str'>") {
                DEBUG("constant at index %lu is string", this->co_consts.size());
                this->co_consts.push_back(
                    std::make_shared<std::string>(element.at("value").get<std::string>())
                );
            } else if (real_type == "<class 'int'>") {
                DEBUG("constant at index %lu is int", this->co_consts.size());
                this->co_consts.push_back(
                    element.at("value").get<int64_t>()
                );
            } else if (real_type == "<class 'float'>") {
                DEBUG("constant at index %lu is float", this->co_consts.size());
                this->co_consts.push_back(
                    element.at("value").get<double>()
                );
            } else if (real_type == "<class 'NoneType'>") {
                DEBUG("constant at index %lu is nonetype", this->co_consts.size());
                this->co_consts.push_back(
                    value::NoneType()
                );
            } else if (real_type == "<class 'bool'>") {
                DEBUG("constant at index %lu is bool", this->co_consts.size());
                this->co_consts.push_back(
                    element.at("value").get<bool>()
                );
            } else {
                throw pyerror(std::string("unrecognized type of constant: ") + real_type);
            }
        } else if (type == "code") {
            this->co_consts.push_back(std::make_shared<Code>(element));
        } else {
            throw pyerror(std::string("unrecognized type of constant: ") + type);
        }
    }

    // load names
    for (const json& name : tree.at("co_names")) {
        DEBUG("loaded name %lu) %s", this->co_names.size(), name.get<std::string>().c_str())
        this->co_names.push_back(
            name.get<std::string>()
        );
    }
}   

Code::~Code() {
}

std::shared_ptr<Code> Code::from_program(const std::string& python, const std::string& compilePyPath) {
    procxx::process compilePyProc{"python3", compilePyPath.c_str()};
    
    DEBUG("execing compile python process");
    compilePyProc.exec();
    compilePyProc << python;
    compilePyProc.close(procxx::pipe_t::write_end());
    
    std::istreambuf_iterator<char> eos;
    std::string outputJson(std::istreambuf_iterator<char>(compilePyProc.output()), eos);
    std::string outputError(std::istreambuf_iterator<char>(compilePyProc.error()), eos);

    if (outputError.size() > 0) {
        std::cout << "ERROR PARSING SOURCE CODE: " << std::endl << outputError;
        throw pyerror("source code error");
    }
    
    DEBUG("trying to read JSON");
    json tree;
    try {
        tree = json::parse(outputJson.c_str());
    } catch (std::exception& err) {
        std::cout << outputJson << std::endl;
        throw err;
    }
    DEBUG("read successful.");
    return std::make_shared<Code>(tree);
}

}
