#ifndef VALUE_H
#define VALUE_H

#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <boost/variant/variant.hpp>
#include <functional>
#include "pyerror.hpp"

namespace py {

// forward declarations
struct Code;
struct FrameState;

// the value namespace for C value types
namespace value {
   
    struct NoneType { };

    struct CFunction;
}

using ValueString = std::shared_ptr<std::string>;
using ValueCode = std::shared_ptr<const Code>;
using ValueCFunction = std::shared_ptr<const value::CFunction>;

using Value = boost::variant<
    int64_t,
    double,
    ValueString,
    ValueCode,
    ValueCFunction,
    value::NoneType
>;

namespace value {
    struct CFunction {
        std::function<void(FrameState&, std::vector<Value>&)> action;
        CFunction(const std::function<void(FrameState&, std::vector<Value>&)>& action) : action(action) { };
    };
}

}

#endif