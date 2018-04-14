#include <functional>
#include <memory>
#include <string>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <iostream>
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
            if (args.size() != 1)
                throw pyerror(string("'print' expected 1 argument"));
            auto value = boost::apply_visitor(value_helper::visitor_repr(), args[0]);
            std::cout << value << std::endl;
        } catch (boost::bad_get& err) {
            throw pyerror(std::string("can not print non-string value ") + args[0].type().name());
        }

        // return None
        frame.value_stack.push(value::NoneType());
        
        return ;
    });
}

}
}