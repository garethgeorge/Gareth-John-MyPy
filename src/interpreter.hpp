#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdint.h>
#include <vector>
#include <stack>
#include <boost/property_tree/ptree.hpp>
#include <boost/variant/variant.hpp>

namespace mypy {

struct Code;

// see https://www.boost.org/doc/libs/1_55_0/doc/html/variant/tutorial.html

using Value = boost::variant<
    int64_t,
    double,
    std::string,
    std::shared_ptr<Code>
>;

struct Code {
    using ByteCode = uint8_t;

    std::vector<Value> constants;
    std::vector<ByteCode> bytecode;
    
    Code(const boost::property_tree::ptree& tree);
    ~Code();
};

struct Block {
    // TODO: eventually there will be values that go in here!
};

struct Frame {
    Frame *parent_frame = nullptr;
    std::shared_ptr<Code> code_obj;
    std::stack<Block *> block_stack;
    std::stack<Value> value_stack;
};

}


#endif 