#include <iostream>
#include <stdexcept>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "interpreter.hpp"
#include "../lib/base64.hpp"

namespace pt = boost::property_tree;

namespace mypy {

Code::Code(const pt::ptree& tree) {
    // decode the bytecode and push it into the bytecode property :)
    std::string bytecode = base64_decode(tree.get<std::string>("co_code"));
    this->bytecode.reserve(bytecode.length());
    for (const unsigned char c : bytecode) {
        this->bytecode.push_back(c);
    }

    const auto& co_consts = tree.get_child("co_consts");

    for (const auto& constValue : co_consts) {
        const auto type = constValue.second.get<std::string>("type");
        if (type == "literal") {
            // std::cout << "literal value: " << constValue.second.get<std::string>("value") << std::endl;
            // std::cout << "TODO: stop interpreting all literals as strings!" << std::endl;
            this->constants.push_back(constValue.second.get<std::string>("value"));
        } else if (type == "code") {
            this->constants.push_back(std::make_shared<Code>(constValue.second));
        } else {
            throw std::runtime_error(std::string("unrecognized type of constant: ") + type);
        }
    }
}

Code::~Code() {
}

}
