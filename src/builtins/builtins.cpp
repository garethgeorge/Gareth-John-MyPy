#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <algorithm>
#include <variant>
#include <math.h>
#include "builtins.hpp"
#include "builtins_helpers.hpp"
#include "../pyvalue_helpers.hpp"
#include "../pyerror.hpp"
#include "../pyframe.hpp"
#include "../pyvalue.hpp"

using std::string;

namespace py {
namespace builtins {


struct float_visitor {
    double operator()(int64_t value) {
        return (double)value;
    }

    double operator()(double value) {
        return (double)value;
    }

    double operator()(auto value) {
        throw pyerror("Can not convert value to int");
    }
};

struct int_visitor {
    int64_t operator()(int64_t value) {
        return value;
    }

    int64_t operator()(double value) {
        return (int64_t)value;
    }

    int64_t operator()(auto value) {
        throw pyerror("Can not convert value to int");
    }
};

extern void inject_builtins(Namespace& ns) {

    // inject the global print builtin
    // TODO: add argument count support
    (*ns)["print"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& args) {
        try {
            for (size_t i = 0; i < args.size(); ++i) {
                const std::string str = std::visit(value_helper::visitor_str(), args[i]);
                std::cout << str;
                if (i != args.size() - 1){
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

    (*ns)["range"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& _args) {
        arg_decoder<int64_t, int64_t, int64_t> args(_args);

        int64_t start, stop, step_size;

        step_size = 1;
        if (_args.size() == 1) {
            start = 0;
            stop = args.get<0>();
        } else if (_args.size() == 2) {
            start = args.get<0>();
            stop = args.get<1>();
        } else if (_args.size() == 3) {
            start = args.get<0>();
            stop = args.get<1>();
            step_size = args.get<2>();
        } else {
            throw pyerror("range takes at most 3 arguments");
        }

        struct range_generator : public value::CGenerator {
            int64_t start;
            int64_t stop;
            int64_t step_size;

            range_generator(int64_t start, int64_t stop, int64_t step_size) {
                this->start = start;
                this->stop = stop;
                this->step_size = step_size;
            }

            virtual std::optional<Value> next() {
                DEBUG_ADV("start: " << start << " stop: " << stop);
                if (start < stop) {
                    int64_t ret = start;
                    start += step_size;
                    return ret;
                } else 
                    return std::nullopt;
            }
        };
        
        Value retval = std::make_shared<range_generator>(start, stop, step_size);

        frame.value_stack.push_back(retval);
    });

    (*ns)["str"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& args) {
        if (args.size() != 1) {
            throw pyerror("ArgError: string takes 1 argument");
        }
        frame.value_stack.push_back(
            alloc.heap_string.make(std::visit(value_helper::visitor_str(), args[0]))
        );
    });

    (*ns)["len"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& _args) {
        arg_decoder<ValueList> args(_args);
        ValueList list = args.get<0>();

        frame.value_stack.push_back(
            (int64_t)list->values.size()
        );
    });

    (*ns)["int"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& _args) {
        if (_args.size() != 1) {
            throw pyerror("ArgError: int takes 1 argument");
        }

        int64_t intValue = std::visit(int_visitor(), _args[0]);
        frame.value_stack.push_back(intValue);
    });

    (*ns)["float"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& _args) {
        if (_args.size() != 1) {
            throw pyerror("ArgError: float takes 1 argument");
        }

        frame.value_stack.push_back(
            std::visit(float_visitor(), _args[0])
        );
    });

    // Returns a proxy object
    (*ns)["super"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& args) {
       DEBUG_ADV("Super called with args " << args[0] << ", " << ((args.size() > 1) ? args[1] : "") << "\n");
    });

    (*ns)["collect_garbage"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& args) {
        alloc.collect_garbage(*(frame.interpreter_state));
        frame.value_stack.push_back(value::NoneType());
    });

    (*ns)["math"] = alloc.heap_namespace.make();

    (*ns)["sqrt"] = pycfunction_builder([](double val) -> double {
        return sqrt(val);
    }).to_pycfunction();

    (*ns)["log"] = pycfunction_builder([](double val, double base) -> double {
        return log(val) / log(base);
    }).to_pycfunction();

    (*ns)["log10"] = pycfunction_builder([](double val, double base) -> double {
        return log10(val);
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
    (*ns)["__build_class__"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& args) {
        #ifdef JOHN_DEBUG_ON
        fprintf(stderr,"__build_class__ called with arguments:\n");
        for(int i = 0;i < args.size();i++){
            frame.print_value(args[i]);
            fprintf(stderr,"\n");
        }
        (std::get<ValuePyFunction>(args[0]))->code->print_bytecode();
        #endif

        /*while (args.hasNext()) {
            std::cout << "ARGUMENT: " << *args << std::endl;
            ++args;
        }

        exit(0);*/

        // Store code
        ValueCode init_code = std::get<ValuePyFunction>(args[0])->code;
        // Store name
        ValueString class_name = std::get<ValueString>(args[1]);

        // There is some unhealthy copying going on here
        // This needs to be fixed and made better
        std::vector<ValuePyClass> tmp_vect;
        tmp_vect.reserve(args.size());

        // Check for type errors
        try{
            for(size_t i = 2; i < args.size(); ++i) {
                tmp_vect.push_back(std::get<ValuePyClass>(args[i]));
            }
        } catch (const std::bad_variant_access& e) {
            throw pyerror("classes can only inherit from classes");
        }

        // Allocate the class
        ValuePyClass new_class = alloc.heap_pyclass.make(tmp_vect);

        // Allocate the class and push it to the top of the stack
        // Args now holds the list of parents
        frame.value_stack.push_back(
            new_class
        );

        // Push the static initializer frame ontop the stack
        // The static initializer code block is the first argument
        frame.interpreter_state->push_frame(
            alloc.heap_frame.make(init_code, new_class)
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
    (*ns)["classmethod"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& args) {
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
                    alloc.heap_pyfunc.make( 
                        value::PyFunc {
                            vpf->name, 
                            vpf->code, 
                            vpf->def_args, 
                            (*vpo)->static_attrs, // self
                            value::CLASS_METHOD
                        }
                    )
                );
            } else {
                if(frame.init_class){
                    // Read the class from the framestate
                    frame.value_stack.push_back(
                        alloc.heap_pyfunc.make( 
                            value::PyFunc {
                                vpf->name,
                                vpf->code,
                                vpf->def_args,
                                frame.init_class, //self
                                value::CLASS_METHOD
                            } 
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
    (*ns)["staticmethod"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& args) {
        if (args.size() != 1) {
            throw pyerror("staticmethod builtin not passed exactly one argument");
        }

        try {
            ValuePyFunction& vpf = std::get<ValuePyFunction>(args[0]);
            frame.value_stack.push_back(
                alloc.heap_pyfunc.make( 
                    // Throw one up on the stack with static flag set
                    value::PyFunc {
                        vpf->name,
                        vpf->code,
                        vpf->def_args,
                        vpf->self,
                        value::STATIC_METHOD
                    }
                )
            );
        } catch (const std::bad_variant_access& e) {
            throw pyerror("staticmethod called on a non function type");
        }
    });

    (*ns)["slice"] = std::make_shared<value::CFunction>([](FrameState& frame, ArgList& args) {
        if(args.size() == 2){
            frame.value_stack.push_back(
                builtins_slice_get_slice_object(args[0],args[1],value::NoneType())
            );
            DEBUG_ADV("pushed the slice: " << frame.value_stack.back());
        } else if(args.size() == 3){
            frame.value_stack.push_back(
                builtins_slice_get_slice_object(args[0],args[1],args[2])
            );
            DEBUG_ADV("pushed the slice: " << frame.value_stack.back());
        } else {
            throw pyerror(
                std::string(
                    "slice called with " + std::to_string(args.size()) + " args, should be 2 or 3"
                )
            );
        }
    });
    
}

}
}