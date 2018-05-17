#include "pyvalue_helpers.hpp"
#include <iostream>
#include <sstream>
#include <stdio.h>

namespace py {
namespace value_helper {
/*
    visitor_is_truthy
*/
bool visitor_is_truthy::operator()(double d) const {
    return d != (double)0;
}

bool visitor_is_truthy::operator()(int64_t d) const {
    return d != (int64_t)0;
}

bool visitor_is_truthy::operator()(const ValueString& s) const {
    return s->size() != 0;
}

bool visitor_is_truthy::operator()(const value::NoneType) const {
    return false;
}


/*
    visitor_debug_repr
*/
struct visitor_debug_repr {
    std::ostream& stream;
    visitor_debug_repr(std::ostream& stream) : stream(stream) { };

    void operator()(bool v) {
        stream << v ? "true" : "false";
    }

    void operator()(double d) {
        stream << d;
    }

    void operator()(int64_t d) {
        stream << d;
    }

    void operator()(ValueString d) {
        stream << *d;
    }

    void operator()(value::NoneType) {
        stream << "None";
    }

    void operator()(ValuePyFunction func) {
        if (func != nullptr) {
            stream << *(func->name);
        } else 
            stream << "anonymous function";
    }

    void operator()(ValuePyClass arg) {
        stream << "ValuePyClass ("
               << *(std::get<ValueString>((*(arg->attrs))["__qualname__"])) << ")";
    }

    void operator()(ValuePyObject arg) {
        stream << "ValuePyObject of class ("
               << *(std::get<ValueString>((*(arg->static_attrs->attrs))["__qualname__"])) << ")";
    }

    void operator()(ValueList list) {
        stream << "[";
        for (auto& value : list->values) {
            std::visit(value_helper::visitor_debug_repr(stream), value);
            stream << ", ";
        }
        stream << "]";
    }

    template<typename T> 
    void operator()(T) const {
        // TODO: use typetraits to generate this.
        throw pyerror("unimplemented repr for type");
    }
};

ValuePyObject create_cell(Value contents){
    DEBUG_ADV("Creating cell for " << contents << "\n");
    ValuePyObject nobj = std::make_shared<value::PyObject>(value::PyObject(cell_class));
    nobj->store_attr("contents",contents);
    return nobj;
}


}

std::ostream& operator << (std::ostream& stream, const Value value) {
    std::visit(value_helper::visitor_debug_repr(stream), value);
    return stream;
}

// This is needed to allow the create_cell function
ValuePyClass cell_class = std::make_shared<value::PyClass>(value::PyClass("CELL_CLASS"));

// PyClass --------------------------------------------------------------------------------------

// Check if we are done computing method resolution order
bool value::PyClass::more_linearization_work_to_do(std::vector<std::vector<ValuePyClass>>& ls){
    for(int i = 0;i < ls.size();i++){
        if(ls[i].size() > 0){
            return true;
        }
    }
    return false;
}

// Which list has a head which is not in the tail of any other list
int value::PyClass::get_next_good_head_ind(std::vector<std::vector<ValuePyClass>>& ls){
    // Loop over every list
    for(int i = 0;i < ls.size();i++){
        // If this is still an interesting list
        if(ls[i].size() > 0){
            // Be optimistic
            bool good_head = true;
            // And for that list, check every other list
            for(int j = 0;j < ls.size();j++){
                // Do not check self
                if(i != j){
                    // Check every tail element of that list to the head of this list
                    for(int k = 1;k < ls[j].size();k++){
                        if(ls[j][k] == ls[i][0]){
                            // This head was in the tail of another list
                            // That means it's no good
                            good_head = false;
                            j = ls.size();
                            break;
                        }
                    }
                }
            }

            // If we got it good
            if(good_head){
                return i;
            }
        }
    }

    // No good head found
    return -1;
}

// Remove a class from being considered in the linearizations
void value::PyClass::remove_from_linearizations(
    std::vector<std::vector<ValuePyClass>>& ls,
    ValuePyClass& cls    
){
    // Loop over every list
    for(int i = 0;i < ls.size();i++){
        for(int j = 0;j < ls[i].size();j++){
            if(ls[i][j] == cls){
                // An item can only appear once in each linearization
                // So find it and remove it if it exists
                ls[i].erase(ls[i].begin() + j);
                break;
            }
        }
    }
}

value::PyClass::PyClass(){
    // Allocate the attributes namespace
    this->attrs = std::make_shared<std::unordered_map<std::string, Value>>();
}

value::PyClass::PyClass(std::string&& qualname) : PyClass() {
    (*(this->attrs))["__qualname__"] = std::make_shared<std::string>(std::string(qualname));
}

value::PyClass::PyClass(std::vector<ValuePyClass>& ps) : PyClass() {

    // Store the list or parents in the correct method resolution order
    // Python3 uses python 2.3's MRO
    //https://www.python.org/download/releases/2.3/mro/
    //L(C(B1...BN)) = C + merge(L(B1)...L(BN),B1..BN)
    // First, create a vector for each class
    // This is very bad, but I want to make sure I understand the math before I make it fast
    if(ps.size() == 1){
        // If there is only one parent, linearization is trivial
        parents.push_back(ps[0]);
        parents.insert(parents.end(),ps[0]->parents.begin(),ps[0]->parents.end());
    } if(ps.size() > 1) {
        std::vector<std::vector<ValuePyClass>> linearizations;
        linearizations.reserve(ps.size());

        // Create the linearizations
        for(int i = 0;i < ps.size();i++){
            // A linearization is the class followed by the rest of the parents
            linearizations.push_back(std::vector<ValuePyClass>());
            linearizations[i].push_back(ps[i]);
            linearizations[i].insert(
                linearizations[i].end(),
                ps[i]->parents.begin(),
                ps[i]->parents.end()
            );
        }

        while(more_linearization_work_to_do(linearizations)){
            int ind = get_next_good_head_ind(linearizations);
            if(ind == -1){
                throw pyerror("Non-ambiguous method resolution order could not be found");
            }
            parents.push_back(linearizations[ind][0]);
            remove_from_linearizations(linearizations,parents.back());
        }
    }
}

}
