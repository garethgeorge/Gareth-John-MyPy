#ifndef PYGC3_H
#define PYGC3_H

#include <list>
#include <functional>
#include <optional>
#include <memory>
#include <stdint.h>

#define DEBUG_ON
#include <debug.hpp>
#undef DEBUG_ON

// we will have to implement a custom allocator that allows us to track the memory
// that is being used, and free'd by our 'MyPy' implementation 
// see https://www.codeproject.com/Articles/4795/C-Standard-Allocator-An-Introduction-and-Implement

namespace gc {

template<typename T>
struct gc_heap;

template<typename T>
class gc_ptr {
protected:
    // the last bit is used to hold a marked flag
    static const uint8_t FLAG_MARKED = 1 << 7;
    // the rest of the bits are used to store a reference count
    // used when C scripts want to retain a value
    static const uint8_t MASK_REFCOUNT = ~FLAG_MARKED;
    // it remains very cheap to check if an object should be gc'd since
    // the gc state is simply when obj.flags = 0

    struct gc_object {
        uint8_t flags = 0;
        T object;

        template < typename... Args> 
        gc_object(Args&&... args) : object(std::forward<Args>(args)...), flags(0) {
        };
    };
    
    gc_object* object;

    gc_ptr(gc_object& object) : object(&object) {
    }

public:
    gc_ptr() : gc_ptr(nullptr) {
    }
    
    gc_ptr(std::nullptr_t) {
        object = nullptr;
    }

    inline operator bool() const {
        return object != nullptr;
    }

    constexpr inline T* operator->() {
        return &(this->object->object);
    }

    constexpr inline T* operator->() const {
        return &(this->object->object);
    }

    constexpr inline T& operator*() const {
        return object->object;
    }

    constexpr bool operator == (const gc_ptr<T> other) const {
        return object == other.object;
    }
    
    constexpr bool operator == (const std::nullptr_t) const {
        return object == nullptr;
    }

    constexpr bool operator != (const gc_ptr<T> other) const {
        return object != other.object;
    }

    constexpr bool operator != (const std::nullptr_t) const {
        return object != nullptr;
    }
    
    void mark() const {
        if (!(this->object->flags & FLAG_MARKED)) {
            this->object->flags |= FLAG_MARKED;
            mark_children(*this);
        }
    }

    void mark() {
        if (!(this->object->flags & FLAG_MARKED)) {
            this->object->flags |= FLAG_MARKED;
            mark_children(*this);
        }
    }

    T* get() {
        if (object == nullptr)
            return nullptr;
        return &(object->object);
    }

    const T* get() const {
        if (object == nullptr)
            return nullptr;
        return &(object->object);
    }

    friend class gc_heap<T>;

    // WARNING: maximum reference count is 127, greater than this and things
    // break horribly, and there is no checking.
    inline gc_ptr<T> retain() {
        if (this->object->flags < 127) {
            this->object->flags += 1;
        }
        return *this;
    }

    void release() {
        if (this->object->flags != 127) {
            this->object->flags -= 1;
        }
    }
};


template<typename T>
class gc_heap {
private:
    // declare a few type aliases to make our code more concise
    using ptr_t = gc_ptr<T>;
    using gc_object = typename ptr_t::gc_object;

    // the objects array is effectively the heap
    std::list<gc_object> objects;
    std::list<gc_object*> rootset;

public:
    template < typename... Args> 
    ptr_t make(Args&&... args) {
        DEBUG("we tried to allocate an object!");
        typename std::list<gc_object>::iterator object_itr = 
            objects.emplace(objects.begin(), std::forward<Args>(args)...);
        return ptr_t(*object_itr);
    }

    size_t size() {
        return objects.size();
    }

    size_t memory_footprint() {
        return size() * sizeof(T);
    }
    
    void sweep() {
        for (auto itr = this->objects.begin(); itr != this->objects.end();) {
            auto &obj = *itr;
            if (!obj.flags) { // object.flags must be all 0's for us to clear it :)
                itr = objects.erase(itr);
            } else {
                (*itr).flags &= ~(ptr_t::FLAG_MARKED);
                ++itr;
            }
        }
    }

    void retain_all() {
        for (auto& object : this->objects) {
            object.flags |= 1;
        }
    }
};



}

#endif 
