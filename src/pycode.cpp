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
#include "oplist.hpp"
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
    this->co_argcount = tree.at("co_argcount").get<uint64_t>();

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

    // load var names
    for (const json& vname : tree.at("co_varnames")) {
        DEBUG("loaded var name %lu) %s", this->co_names.size(), vname.get<std::string>().c_str())
        this->co_varnames.push_back(
            vname.get<std::string>()
        );
    }

    // load lnotab 
    for (const json& linenumber : tree.at("lnotab")) {
        DEBUG("loaded line number %d", linenumber);
        this->lnotab.push_back(
            LineNoMapping {linenumber.at(0).get<uint64_t>(), linenumber.at(1).get<uint64_t>()}
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

// Simplistically print out every opcode
void Code::print_bytecode() const {
    for(int i = 0;i < bytecode.size();){
        printf("%u = %s",bytecode[i],op::name[bytecode[i]]);
        if(bytecode[i] >= op::HAVE_ARGUMENT){
            uint32_t arg = bytecode[i + 2];
            arg = (arg << 8) | bytecode[i + 1];
            printf(" %lu\n",arg);
            i += 3;
        } else {
            printf("\n");
            i += 1;
        }
    }
}

}
