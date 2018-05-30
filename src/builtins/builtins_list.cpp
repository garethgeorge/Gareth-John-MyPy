#include <iostream>

#include "builtins.hpp"
#include "builtins_helpers.hpp"

// #include "../pyvalue_helpers.hpp"

namespace py {
namespace builtins {

std::unordered_map<std::string, ValueCMethod> builtin_list_attributes;

void initialize_list_class() {
    // builtin_list_attributes["append"] = pycfunction_builder([](ValueList& list, Value val) -> void {
    //     list->values.push_back(val);
    // }).to_pycmethod();

    builtin_list_attributes["append"] = std::make_shared<value::CMethod>([](FrameState& frame, ArgList& _args) {
        arg_decoder<ValueList> args(_args);

        if (_args.size() < 2) {
            throw pyerror("list.append expected 2 arguments.");
        }
        args.get<0>()->values.push_back(_args[1]);

        frame.value_stack.push_back(value::NoneType());
    });

    builtin_list_attributes["extend"] = std::make_shared<value::CMethod>([](FrameState& frame, ArgList& _args) {
        arg_decoder<ValueList, ValueList> args(_args);
        ValueList list = args.get<0>();
        
        for (auto& val : args.get<1>()->values) {
            list->values.push_back(val);
        }

        frame.value_stack.push_back(value::NoneType());
    });

    builtin_list_attributes["insert"] = std::make_shared<value::CMethod>([](FrameState& frame, ArgList& _args) {
        arg_decoder<ValueList, int64_t> args(_args);

        ValueList list = args.get<0>();
        int64_t index = args.get<1>();
        if (_args.size() < 3) {
            throw pyerror("list.insert expected 3 arguments.");
        }
        
        Value val = _args[2];

        if (index < 0 || index >= list->values.size()) {
            throw pyerror("RangeError: index is out of range.");
        }

        list->values.insert(list->values.begin() + index, val);

        frame.value_stack.push_back(value::NoneType());
    });

}

}
}