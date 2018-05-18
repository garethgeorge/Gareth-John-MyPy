#include <iostream>

#include "builtins.hpp"
#include "builtins_helpers.hpp"

// #include "../pyvalue_helpers.hpp"

namespace py {
namespace builtins {

std::unordered_map<std::string, ValueCMethod> builtin_list_attributes;

struct builtin_list_attributes_initializer {
    builtin_list_attributes_initializer() {
        std::cout << "INITIALIZING BUILTIN_LIST_ATTRIBUTES" << std::endl;

        builtin_list_attributes["append"] = pycfunction_builder([](ValueList& list, Value val) -> void {
            list->values.push_back(val);
        }).to_pycmethod();

        builtin_list_attributes["extend"] = pycfunction_builder([](ValueList& list, const ValueList& extender) -> void {
            list->values.reserve(list->values.size() + extender->values.size());
            for (auto& val : extender->values) {
                list->values.push_back(val);
            }
        }).to_pycmethod();


        builtin_list_attributes["insert"] = pycfunction_builder([](ValueList& list, int64_t index, Value& val) -> void {
            if (index < 0 || index >= list->values.size()) {
                throw pyerror("RangeError: index is out of range.");
            }

            list->values.insert(list->values.begin() + index, val);
        }).to_pycmethod();

    }
};

builtin_list_attributes_initializer builtin_list_attributes_initializer;

}
}