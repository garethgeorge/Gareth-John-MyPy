#ifndef PYGC3_H
#define PYGC3_H

#include <list>
#include <functional>
#include <optional>
#include <memory>
#include <stdint.h>

// we will have to implement a custom allocator that allows us to track the memory
// that is being used, and free'd by our 'MyPy' implementation 
// see https://www.codeproject.com/Articles/4795/C-Standard-Allocator-An-Introduction-and-Implement

namespace gc {

template<typename T>
struct gc_heap;

template<typename T>
class gc_ptr {
private:
    static const uint8_t FLAG_MARKED = 1;

    struct gc_object {
        uint8_t flags;
        T object;

        template < typename... Args> 
        gc_object(Args&&... args) : object(std::forward<Args>(args)...), flags(0) {
        };
    };
    
    gc_object* object;

    gc_ptr(gc_object& object) : object(&object) {
    }

public:
    gc_ptr(gc_ptr<T> &other) {
        *this = other;
    }

    inline T* operator->() {
        return &(this->object->object);
    }

    inline T& operator*() {
        return object->object;
    }

    T& operator = (gc_ptr<T>& other) {
        this->object = other.object;
    }
    
    void mark() {
        if (!(this->object->flags & FLAG_MARKED)) {
            this->object->flags |= FLAG_MARKED;
            mark_children(*this);
        }
    }

    friend class gc_heap<T>;
};


template<typename T>
class gc_heap {
private:
    // declare a few type aliases to make our code more concise
    using ptr_t = gc_ptr<T>;
    using gc_object = typename ptr_t::gc_object;

    // the objects array is effectively the heap
    std::list<gc_object> objects;

public:
    template < typename... Args> 
    ptr_t make(Args&&... args) {
        typename std::list<gc_object>::iterator object_itr = 
            objects.emplace(objects.begin(), std::forward<Args>(args)...);
        return ptr_t(*object_itr);
    }


    size_t size() {
        return objects.size();
    }
    
    void sweep() {
        for (auto itr = this->objects.begin(); itr != this->objects.end();) {
            auto &obj = *itr;
            if (!(obj.flags & ptr_t::FLAG_MARKED)) {
                objects.erase(itr++);
            } else {
                (*itr).flags &= ~(ptr_t::FLAG_MARKED);
                ++itr;
            }
        }
    }
};

}

#endif 
