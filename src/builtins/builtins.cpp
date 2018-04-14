#include <functional>
#include <memory>
#include <string>
#include <boost/variant/variant.hpp>
#include <boost/variant/get.hpp>
#include <iostream>
#include "builtins.hpp"

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns) {
    
    // inject the global print builtin
    ns["print"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        try {
            auto value = boost::get<std::shared_ptr<std::string>>(args[0]);
            std::cout << *value << std::endl;
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