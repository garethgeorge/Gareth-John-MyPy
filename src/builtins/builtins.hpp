#ifndef BUILTINS_H
#define BUILTINS_H

#include "../pyinterpreter.hpp"
#include <type_traits>
#include <functional>
#include <iostream>

namespace py {
namespace builtins {

extern void inject_builtins(Namespace& ns);

template<typename T> 
struct function_traits;  

template<typename R, typename ...Args> 
struct function_traits<std::function<R(Args...)>>
{
    static const size_t nargs = sizeof...(Args);

    typedef R result_type;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

template<typename R>
struct make_cfunction {
    R func;

    make_cfunction(R func) : func(func) {
    }


    std::function<void(FrameState&, std::vector<Value>&)> wrap_in_function() {
        auto copy = *this;

        return [copy](FrameState& frm, std::vector<Value>& vals) -> void {
            copy(frm, vals);
            return ;
        };
    }

    Value operator()(FrameState& frame, std::vector<Value>& args) {
        using traits = function_traits<decltype(func)>;
        using result_type = typename traits::result_type;

        if constexpr(std::is_void<result_type>::value) {
            std::cout << "the function returns NoneType" << std::endl;
            func();
            frame.value_stack.push_back(value::NoneType());
        } else {

        }


        // if constexpr(std::is_same<return_type, void>::value) {
        //     T()();
        //     frame.value_stack.push_back(value::NoneType());
        // } else {
        //     return_type val = T()();
        //     frame.value_stack.push_back(val);
        // }
    }

};

}
}

#endif