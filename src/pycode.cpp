#include <iostream>
#include <stdexcept>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "pyinterpreter.hpp"
#include "pyvalue_types.hpp"
#include "../lib/base64.hpp"

namespace pt = boost::property_tree;

namespace py {

Code::Code(const pt::ptree& tree) {
    // decode the bytecode and push it into the bytecode property :)
    std::string base64bytecode = tree.get<std::string>("co_code");
    std::string bytecode = base64_decode(base64bytecode);
    this->bytecode.reserve(bytecode.length());
    
    for (const unsigned char c : bytecode) {
        this->bytecode.push_back(c);
    }

    const auto& co_consts = tree.get_child("co_consts");

    for (const auto& constValue : co_consts) {
        const auto type = constValue.second.get<std::string>("type");
        if (type == "literal") {
            // TODO: make this properly handle the type information for the 
            // constant literal
            const auto tmp = std::make_shared<StringLiteral>(
                constValue.second.get<std::string>("value")
            );
            this->constants.push_back(tmp);
        } else if (type == "code") {
            // this->constants.push_back(std::make_shared<Code>(constValue.second));
        } else {
            throw std::runtime_error(std::string("unrecognized type of constant: ") + type);
        }
    }
}

Code::~Code() {
}

}
