#ifndef VALUE_TYPES_H
#define VALUE_TYPES_H

#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <boost/variant/variant.hpp>
#include <type_traits>
#include "pyvalue.hpp"

namespace py {


struct LiteralValue : Value {
    using MyType = boost::variant<int64_t, double, std::string>;
    const MyType raw_value;

    template<typename T>
    LiteralValue(const T& v) : raw_value(v) { };

    virtual Type getType() override {
        return 1;
    }

    struct add_visitor: public boost::static_visitor<MyType> {
        MyType operator()(double v1, double v2) const {
            return v1 + v2;
        }
        MyType operator()(double v1, int64_t v2) const {
            return v1 + v2;
        }
        MyType operator()(int64_t v1, double v2) const {
            return v1 + v2;
        }
        MyType operator()(int64_t v1, int64_t v2) const {
            return v1 + v2;
        }

        MyType operator()(const std::string& v1, const std::string &v2) const {
            return v1 + v2;
        }

        template<typename T1, typename T2>
        MyType operator()(T1, T2) const {
            throw pyerror("type error in add");
        }
    };
    
    virtual std::shared_ptr<Value> add(std::shared_ptr<Value>& other) override {
        if (other->getType() == this->getType()) {
            LiteralValue *otherCasted = dynamic_cast<LiteralValue *>(other.get());
            MyType result = boost::apply_visitor(add_visitor(), this->raw_value, otherCasted->raw_value);
            return std::make_shared<LiteralValue>(result);
        } else {
            throw pyerror("TypeError in add");
        }
    }
};

struct StringValue : Value {
    const std::string raw_value;

    StringValue(const std::string& value) : raw_value(value) { };

    virtual Type getType() override {
        return 2;
    }

    virtual std::shared_ptr<Value> add(std::shared_ptr<Value>& other) override {
        StringValue *otherStringValue = dynamic_cast<StringValue *>(other.get());
        if (otherStringValue != nullptr) {
            return std::make_shared<StringValue>(this->raw_value + otherStringValue->raw_value);
        } else {
            throw pyerror("can only add string with another string.");
        }
        
    }
};

struct NoneValue : Value {
    virtual Type getType() override {
        return 3;
    }
};

}

#endif