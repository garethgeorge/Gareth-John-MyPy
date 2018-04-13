#ifndef VALUE_TYPES_H
#define VALUE_TYPES_H

#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <boost/variant/variant.hpp>
#include "pyvalue.hpp"

namespace py {

struct LiteralValue : Value {
    using MyType = boost::variant<int64_t, double, float>;
    MyType raw_value;

    template<typename T>
    LiteralValue(const T& v) : raw_value(v) { };

    virtual Type getType() override {
        return 1;
    }

    struct add_visitor: public boost::static_visitor<MyType> {
        template <class T1, class T2>
        MyType operator()(T1 v1, T2 v2) const {
            return v1 + v2;
        }
    };

    virtual std::shared_ptr<Value> add(std::shared_ptr<Value>& other) override {
        if (other->getType() == this->getType()) {
            LiteralValue *otherCasted = dynamic_cast<LiteralValue *>(other.get());
            MyType result = boost::apply_visitor(add_visitor(), this->raw_value, otherCasted->raw_value);

            return std::make_shared<LiteralValue>(result);
        } else {
            throw std::runtime_error("TypeError in add");
        }
    }
};

struct StringLiteral : Value {
    std::string raw_value;

    StringLiteral(const std::string& value) : raw_value(value) { };

    virtual Type getType() override {
        return 2;
    }

    virtual std::shared_ptr<Value> add(std::shared_ptr<Value>& other) override {
        return std::make_shared<StringLiteral>("unimplemented string addition");
    }
};

}

#endif