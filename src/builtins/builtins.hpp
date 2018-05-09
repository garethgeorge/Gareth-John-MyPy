#ifndef BUILTINS_H
#define BUILTINS_H

#include "../pyinterpreter.hpp"
#include "../pyvalue_helpers.hpp"
#include <sstream>
#include <type_traits>
#include <functional>
#include <variant>

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns);

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};
// For generic types, directly use the result of the signature of its 'operator()'

// helper struct for getting type information for a callable
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
// we specialize for pointers to member function
{
    enum { arity = sizeof...(Args) };
    // arity is the number of arguments.

    typedef ReturnType result_type;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
        // the i-th argument is equivalent to the i-th tuple element of a tuple
        // composed of those arguments.
    };
};


// helper struct for building up an argpack of list indices
template <std::size_t... Indices>
struct indices {};

template <std::size_t N, std::size_t... Is>
struct build_indices
    : build_indices<N-1, N-1, Is...> {};

template <std::size_t... Is>
struct build_indices<0, Is...> : indices<Is...> {};


// helper struct for calling a function given a vector of py::Value
template <typename traits>
struct unpack_caller
{
    static constexpr size_t num_args = traits::arity;
    std::vector<Value>& args;
    FrameState *frame;

    // we construct the unpack caller with the args it will attempt to apply
    unpack_caller(std::vector<Value>& args) : args(args) {
        if (args.size() != num_args) {
        }
    };

    // transform argument
    template<size_t index>
    auto&& transform() {
        typedef typename traits::template arg<index>::type argType;
        if constexpr(std::is_same<typename std::decay<argType>::type, FrameState>::value) {
            return *frame;
        } else if constexpr(std::is_same<typename std::decay<argType>::type, Value>::value) {
            return args[index];
        } else if constexpr(std::is_same<typename std::decay<argType>::type, typename std::vector<Value>>::value) {
            return args;
        } else {
            if (args.size() <= index) {
                throw pyerror("ArgError: CFunction no argument at index " + std::to_string(index));
            }
            try {
                return std::get<typename std::decay<argType>::type>(args[index]);
            } catch (std::bad_variant_access& e) {
                std::stringstream ss;
                ss << "TypeError: CFunction expected argument #" << index << " to have type "
                << typeid(argType).name() << " but instead got value " << args[index];
                throw pyerror(ss.str());
            }
        }
    }
    
    // internal call method
    template <typename FuncType, size_t... I>
    auto call(FuncType &f, indices<I...>){
        return f(transform<I>()...);
    }

    // the one people should actually use
    template <typename FuncType>
    auto operator () (FuncType &f){
        return call(f, build_indices<num_args>{});
    }
};

template<typename R>
class pycfunction_builder {

    R func;

public:
    pycfunction_builder(R func) : func(func) {
    };

    auto to_pycfunction() {
        return std::make_shared<value::CFunction>([*this](FrameState& frm, std::vector<Value>& vals) -> void {
            (*this)(frm, vals);
            return ;
        });
    }

    Value operator()(FrameState& frame, std::vector<py::Value>& args) const {
        typedef function_traits<decltype(func)> traits;

        auto caller = unpack_caller<traits>(args);
        caller.frame = &frame;
        
        if constexpr(std::is_void<typename traits::result_type>::value) {
            caller(func);
            frame.value_stack.push_back(value::NoneType());
        } else {
            frame.value_stack.push_back(caller(func));
        }
    }

};

}
}

#endif