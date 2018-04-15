#include <functional>
#include <memory>
#include <string>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <iostream>
#include <algorithm>
#include "builtins.hpp"
#include "../pyvalue_helpers.hpp"

using std::string;

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns) {
    
    // inject the global print builtin
    // TODO: add argument count support
    ns["print"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        try {
            std::vector<std::string> strings;
            strings.resize(args.size());

            for (auto it = args.rbegin(); it != args.rend(); ++it) {
                const std::string str = boost::apply_visitor(value_helper::visitor_str(), *it);
                std::cout << str;
                if (it + 1 != args.rend()) {
                    std::cout << " ";
                }
            }
            std::cout << std::endl;
        } catch (boost::bad_get& err) {
            throw pyerror(std::string("can not print non-string value ") + args[0].type().name());
        }
        
        // return None
        frame.value_stack.push(value::NoneType());
        
        return ;
    });

    ns["str"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        if (args.size() > 1) {
            throw pyerror("str in mypy does not support unicode string decoding at the moment.");
        }

        frame.value_stack.push(
            std::make_shared<std::string>(boost::apply_visitor(value_helper::visitor_str(), args[0]))
        );
    });
}

}
}