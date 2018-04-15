#include <iostream>
#include <stdexcept>
#include <sstream>
#include <boost/process.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <vector>
#include <algorithm>

#include "pycode.hpp"
#include "pyvalue.hpp"
#include "../lib/base64.hpp"
#include "../lib/debug.hpp"

namespace pt = boost::property_tree;
namespace bp = boost::process;

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
            } else if (real_type == "<class 'bool'>") {
                DEBUG("constant at index %lu is bool", this->co_consts.size());
                this->co_consts.push_back(
                    constValue.second.get<bool>("value")
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

std::shared_ptr<Code> Code::fromProgram(const std::string& python, const std::string& compilePyPath) {
    bp::pipe feed_in;
    bp::pipe feed_out;
    DEBUG("spawning off child process");
    bp::child c(std::string("python3 ") + compilePyPath, bp::std_out > feed_out, bp::std_in < feed_in);
    feed_in.write(python.c_str(), python.size());
    feed_in.close();

    std::stringstream ss;
    std::vector<char> buffer;
    buffer.resize(1024, '\0');
    while (feed_out.is_open() && feed_out.read(&(buffer.front()), buffer.size()-1) != 0) {
        ss << &(buffer.front());
        std::fill(buffer.begin(), buffer.end(), '\0');
    }
    // DEBUG("compiled python to JSON %s", ss.str().c_str());

    pt::ptree root;
    try {
        DEBUG("trying to read JSON");
        pt::read_json(ss, root);
        DEBUG("read successful.");
        return std::make_shared<Code>(root);
    } catch (pt::json_parser::json_parser_error error) {
        pyerror("error parsing json input from pytools/compile.py");
        return nullptr;
    }
}

}
