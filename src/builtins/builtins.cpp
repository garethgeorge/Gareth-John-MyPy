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
#include "../pyvalue.hpp"

using std::string;

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns) {
    
    // inject the global print builtin
    // TODO: add argument count support
    (*ns)["print"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        try {
            for (auto it = args.begin(); it != args.end(); ++it) {
                const std::string str = std::visit(value_helper::visitor_str(), *it);
                std::cout << str;
                if (it + 1 != args.end()) {
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

    (*ns)["str"] = pycfunction_builder([] (Value value) {
        return std::make_shared<std::string>(std::move(std::visit(value_helper::visitor_str(), value)));
    }).to_pycfunction();

    (*ns)["len"] = pycfunction_builder([] (ValueList list) {
        return (int64_t) list->values.size();
    }).to_pycfunction();

    (*ns)["range"] = pycfunction_builder([] (int64_t range, FrameState& frame) {
        ValueList list = frame.interpreter_state->alloc.heap_lists.make();
        list->values.resize(range);
        for (int64_t i = 0; i < range; ++i) {
            list->values[i] = i;
        }
        return list;
    }).to_pycfunction();

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
    // See the RETURN_VALUE opcode in pyframe.cpp for the final allocation
    // Actually allocating a new PyObject from a PyClass happens in CALL_FUNCTION later
    (*ns)["__build_class__"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        #ifdef JOHN_DEBUG_ON
        fprintf(stderr,"__build_class__ called with arguments:\n");
        for(int i = 0;i < args.size();i++){
            frame.print_value(args[i]);
            fprintf(stderr,"\n");
        }
        (std::get<ValuePyFunction>(args[0]))->code->print_bytecode();
        #endif

        // Store code
        ValueCode init_code = std::get<ValuePyFunction>(args[0])->code;
        args.erase(args.begin());

        // Store name
        ValueString class_name = std::get<ValueString>(args[0]);
        args.erase(args.begin());

        // There is some unhealthy copying going on here
        // This needs to be fixed and made better
        std::vector<ValuePyClass> tmp_vect;
        tmp_vect.reserve(args.size());

        // Check for type errors
        try{
            for(int i = 0;i < args.size();i++){
                tmp_vect.push_back(std::get<ValuePyClass>(args[i]));
            }
        } catch (const std::bad_variant_access& e) {
            throw pyerror("classes can only inherit from classes");
        }

        // Allocate the class
        ValuePyClass new_class = std::make_shared<value::PyClass>(value::PyClass(
            tmp_vect
        ));

        // Allocate the class and push it to the top of the stack
        // Args now holds the list of parents
        frame.value_stack.push_back(
            new_class
        );

        // Push the static initializer frame ontop the stack
        // The static initializer code block is the first argument
        frame.interpreter_state->push_frame(
            std::make_shared<FrameState>(init_code, new_class)
        );

        // Add it's name to it's local namespace
        // Remember that this actually adds it to the class
        frame.interpreter_state->cur_frame->add_to_ns_local("__name__",class_name);

        // No need to initialize from pyfunc, it has no arguments (I think this is always true?)
        // frame.interpreter_state->callstack.top().initialize_from_pyfunc(std::get<ValuePyFunction>(args[0]),std::vector());
        
    });

    // Used to create a class method from an instance method
    // For class methods, 'self' refers to the PyClass a PyObject is an instance of
    // Returns a class method from an instance method
    // Call be called from within a static intinializing frame which, currently, does not 
    // have a reference to the class (it doesnt exist yet)
    // This means flags need to be involved
    // Is there a better way to do this without addng a field to FrameState?
    (*ns)["classmethod"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        if (args.size() != 1) {
            throw pyerror("classmethod builtin not passed exactly one argument");
        }
        
       // Get a version of this function that is a class function
        // If I already know self, set it to self's base class in the new one
        // Otherwise merely set the flag and defer self until we know it (MEEEEH)
        // A different solution to the above line would be to have a static initializer
        // frame state be able to refer to the class it is creating while creating it
        // Is that better?
        // This class if fully defined below
        // The 1 below sets the 'class_method' flag
        
        try {
            ValuePyFunction& vpf = std::get<ValuePyFunction>(args[0]);
            
            // Check that this an instance method
            if(std::get_if<ValuePyClass>(&(vpf->self)) != NULL){
                throw pyerror("classmethod builtin called on a function that is not an instance method");
            }

            // Get the object self refers to
            auto vpo = std::get_if<ValuePyObject>(&(vpf->self));
            if(vpo != NULL){
                // Create a function with self one level deeper
                frame.value_stack.push_back(
                    std::move(
                        std::make_shared<value::PyFunc>( 
                            value::PyFunc {
                                vpf->name, 
                                vpf->code, 
                                vpf->def_args, 
                                (*vpo)->static_attrs, // self
                                value::CLASS_METHOD
                            }
                        )
                    )
                );
            } else {
                if(frame.init_class){
                    // Read the class from the framestate
                    frame.value_stack.push_back(
                        std::move(
                            std::make_shared<value::PyFunc>( 
                                value::PyFunc {
                                    vpf->name,
                                    vpf->code,
                                    vpf->def_args,
                                    frame.init_class, //self
                                    value::CLASS_METHOD
                                } 
                            )
                        )
                    );
                } else {
                    throw pyerror("classmethod builtin called with bad args");
                }
            }
        } catch (const std::bad_variant_access& e) {
            throw pyerror("classmethod builtin called on a function that is not an instance method");
        }
    });

    // Makes a method static!
    (*ns)["staticmethod"] = std::make_shared<value::CFunction>([](FrameState& frame, std::vector<Value>& args) {
        if (args.size() != 1) {
            throw pyerror("staticmethod builtin not passed exactly one argument");
        }

        try {
            ValuePyFunction& vpf = std::get<ValuePyFunction>(args[0]);
            frame.value_stack.push_back(
                std::move(
                    std::make_shared<value::PyFunc> ( 
                        // Throw one up on the stack with static flag set
                        value::PyFunc {
                            vpf->name,
                            vpf->code,
                            vpf->def_args,
                            vpf->self,
                            value::STATIC_METHOD
                        }
                    )
                )
            );
        } catch (const std::bad_variant_access& e) {
            throw pyerror("staticmethod called on a non function type");
        }
    });
}

}
}