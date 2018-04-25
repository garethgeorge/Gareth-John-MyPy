#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <algorithm>
#include <variant>
#include "builtins.hpp"
#include "../pyvalue_helpers.hpp"
#include "../pyerror.hpp"
#include "../pyframe.hpp"

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

    // Build a re[resentation of a class
    // Python creates classes via:
    //  LOAD_BUILD_CLASS
    //  LOAD_CONST (the code object describing the class)
    //  LOAD_CONST (class name)
    //  MAKE_FUNCTION
    //  LOAD_CONST(class name)
    //  CALL_FUNCTION
    //  STORE_NAME (class name)
    // This means that __build_class__ must create something and push it to the stack
    // that, when called later via CALL_FUNCTION creates and pushes on the stack
    // and instance of the class it represents
    ns["__build_class__"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        fprintf(stderr,"__build_class__ called with arguments:\n");
        for(int i = 0;i < args.size();i++){
            frame.print_value(args[i]);
            fprintf(stderr,"\n");
        }
        (std::get<ValuePyFunction>(args[0]))->code->print_bytecode();
        
        // Push the static initializer frame ontop the stack
        // The static initializer code block is the first argument
        frame.interpreter_state->callstack.push(
            std::move(FrameState(frame.interpreter_state, &frame, std::get<ValuePyFunction>(args[0])->code))
        );
        // Add it's name to it's local namespace
        // Name is passed in as the second argument
        frame.interpreter_state->callstack.top().add_to_ns_local("__name__",std::get<ValueString>(args[1]).get());
        
        // This frame state is initializing the statics of a class
        frame.interpreter_state->callstack.top().set_class_static_init_flag();
        
        // No need to do so, it has no arguments (I think this is always true?)
        //frame.interpreter_state->callstack.top().initialize_from_pyfunc(std::get<ValuePyFunction>(args[0]),std::vector());
        
    });
}

}
}