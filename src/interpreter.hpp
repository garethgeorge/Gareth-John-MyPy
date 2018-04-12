#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdint.h>
#include <vector>
#include <stack>
#include <boost/property_tree/ptree.hpp>

namespace mypy {

struct Value {
    // TODO: actually make this a virtual class and have
    // literal types inherit from it ?
    // Don't forget to mark destructor virtual
    enum Type {
        NONE,
        INT,
        FLOAT,
    };

    Type type = Type::NONE;
    uint64_t value = 0;

    static Value fromJSONConstant(const boost::property_tree::ptree& tree);
};

struct Code {
    using ByteCode = uint8_t;
    
    struct CodeConstant {
        enum Type {
            CODE,
            LITERAL
        };
        Type type;
        std::shared_ptr<Code> code = nullptr;
        std::shared_ptr<Value> value = nullptr;
    };

    std::vector<CodeConstant> constants;
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