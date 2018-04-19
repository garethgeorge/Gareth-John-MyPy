#ifndef PYGC_H
#define PYGC_H

#include <stdint.h>
#include <list>

namespace py {
namespace gc {

template<class T>
class gc_heap<T>;

template<class T>
class gc_ptr {
private:
    struct gc_heap_object {
        gc_heap* owner;
        size_t refcount;
        std::list<gc_wrapped<T>>::iterator gc_heap_itr;
        T real;
    }
    
    gc_heap_object* heap_obj;
public:
    operator T* operator->() {
        return &(this->heap_obj->real);
    }

    operator T& operator*() {
        return this->heap_obj->real;
    }

    gc_ptr(Wrapper *wrapper) : wrapper(wrapper) {
        wrapper->refcount++;
    }

    ~gc_ptr() {
        wrapper->refcount--;
        if (wrapper->refcount == 0) {
            // we should be deleting our classes 
            // automatically if we can based on refcount,
            // otherwise we will have to wait for cycle
            // detection via mark and sweep to mark what 
            // can indeed be deleted
            delete this->wrapper;
        }
    }

    template<class U>
    friend class gc_heap<U>;    
};

// template<class T>
// class gc_heap {
//     std::list<Wrapper<T>*> heap;

//     template<typename Args...>
//     static make(Args&&... args) {
//         gc_ptr::Wrapper* = new wrapped;
//         wrapped->internal = std::move(T(std::forward(args)...));
//         wrapped->gc_heap_itr = this->heap->begin() + 0;
//         wrapped->gc_heap = this;
//         this->heap.push_front(wrapped);
//         return gc_ptr<wrapped>();
//     }

//     ~gc_heap() {
//         for (auto val : heap) {
//             delete val;
//         }
//     }
// };


}
}

#endif