#include <iostream>
#include <stdexcept>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "pyinterpreter.hpp"
#include "pyvalue.hpp"
#include "../lib/base64.hpp"

#define DEBUG_ON
#include "../lib/debug.hpp"

namespace pt = boost::property_tree;

namespace py {

Code::Code(const pt::ptree& tree) {
    DEBUG("loading in source code from json");
    // decode the bytecode and push it into the bytecode property :)
    std::string base64bytecode = tree.get<std::string>("co_code");
    std::string bytecode = base64_decode(base64bytecode);
    this->bytecode.reserve(bytecode.length());
    
    for (const unsigned char c : bytecode) {
        this->bytecode.push_back(c);
    }

    // load constants
    const auto& co_consts = tree.get_child("co_consts");
    DEBUG("loading in the co_consts constant pool");
    for (const auto& constValue : co_consts) {
        const auto type = constValue.second.get<std::string>("type");
        if (type == "literal") {
            const auto real_type = constValue.second.get<std::string>("real_type");
            if (real_type == "<class 'str'>") {
                DEBUG("constant at index %lu is string", this->co_consts.size());
                this->co_consts.push_back(
                    std::make_shared<std::string>(constValue.second.get<std::string>("value"))
                );
            } else if (real_type == "<class 'int'>") {
                DEBUG("constant at index %lu is int", this->co_consts.size());
                this->co_consts.push_back(
                    constValue.second.get<int64_t>("value")
                );
            } else if (real_type == "<class 'float'>") {
                DEBUG("constant at index %lu is float", this->co_consts.size());
                this->co_consts.push_back(
                    constValue.second.get<double>("value")
                );
            } else if (real_type == "<class 'NoneType'>") {
                DEBUG("constant at index %lu is nonetype", this->co_consts.size());
                this->co_consts.push_back(
                    value::NoneType()
                );
            } else {
                throw pyerror(std::string("unrecognized type of constant: ") + real_type);
            }
        } else if (type == "code") {
            this->co_consts.push_back(std::make_shared<Code>(constValue.second));
        } else {
            throw pyerror(std::string("unrecognized type of constant: ") + type);
        }
    }

    // load names
    for (const auto& name : tree.get_child("co_names")) {
        DEBUG("loaded name %lu) %s", this->co_names.size(), name.second.get_value<std::string>().c_str())
        this->co_names.push_back(
            name.second.get_value<std::string>()
        );
    }
}   

Code::~Code() {
}

}
