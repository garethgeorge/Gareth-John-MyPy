#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <algorithm>
#include <variant>
#include "builtins.hpp"
#include "../pyvalue_helpers.hpp"
#include "../pyerror.hpp"

using std::string;

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns) {
    
    // inject the global print builtin
    // TODO: add argument count support
    ns["print"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        try {
            for (auto it = args.rbegin(); it != args.rend(); ++it) {
                const std::string str = std::visit(value_helper::visitor_str(), *it);
                std::cout << str;
                if (it + 1 != args.rend()) {
                    std::cout << " ";
                }
            }
            std::cout << std::endl;
        } catch (std::bad_variant_access& err) {
            throw pyerror(std::string("can not print non-string value"));
        }
        
        // return None
        frame.value_stack.push_back(value::NoneType());
        
        return ;
    });

    ns["str"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        if (args.size() != 1) {
            throw pyerror("str in mypy does not support unicode string decoding at the moment.");
        }
        
        frame.value_stack.push_back(
            std::make_shared<std::string>(std::visit(value_helper::visitor_str(), args[0]))
        );
    });

    ns["len"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        if (args.size() != 1) {
            throw pyerror("len() only takes one argument");
        }

        try {
            ValueList value = std::get<ValueList>(args[0]);
            frame.value_stack.push_back((int64_t)(value->values.size()));
        } catch (std::bad_variant_access& err) {
            pyerror("expected a list, got bad datatype");
        }
    });
}

}
}