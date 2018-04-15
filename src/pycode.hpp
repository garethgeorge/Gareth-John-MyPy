#ifndef PYCODE_H
#define PYCODE_H

#include <memory>
#include <vector>
#include <string>

#include <boost/property_tree/ptree_fwd.hpp>
#include "pyvalue.hpp"

namespace py {

struct Code {
    using ByteCode = uint8_t;

    std::vector<ByteCode> bytecode;
    std::vector<Value> co_consts;
    std::vector<std::string> co_names;
    
    Code(const boost::property_tree::ptree& tree);
    ~Code();

    static std::shared_ptr<Code> fromProgram(const std::string& python, const std::string& compilePyPath);
};

}

#endif