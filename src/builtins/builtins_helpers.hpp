#ifndef BUILTINS_HELPERS_HPP
#define BUILTINS_HELPERS_HPP

#include <sstream>
#include "../pyvalue.hpp"
#include "./builtins_helpers_old.hpp"


namespace py {
    template<typename... Args>
    struct arg_decoder {
        template<class... Types>
        struct _count {
            static const std::size_t value = sizeof...(Types);
        };

        static constexpr const size_t expected_arg_count = _count<Args...>::value;

        ArgList& arglist;
        arg_decoder(ArgList& args) : arglist(args) {
        }

        template<size_t index> 
        auto get() {
            // TODO: refactor this for performance reasons
            typedef typename std::tuple_element<index, std::tuple<Args...>>::type argType;
            if (index >= this->arglist.size()) {
                std::stringstream ss;
                ss << "Expected " << expected_arg_count << " arguments, but got " << arglist.size() << " arguments.";
                throw pyerror(ss.str());
            }

            return std::get<argType>(arglist[index]);
        }

        template<size_t index, typename T> 
        auto get(T value) {
            // TODO: refactor this for performance reasons
            typedef typename std::tuple_element<index, std::tuple<Args...>>::type argType;
            static_assert(std::is_same<typename std::decay<T>::type, typename std::decay<argType>::type>::value,
                "default value and argtype for index must match");

            if (index >= this->arglist.size()) {
                return value;
            }

            return std::get<argType>(arglist[index]);
        }
    };
}


#endif