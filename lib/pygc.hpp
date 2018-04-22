#ifndef PYGC_H
#define PYGC_H

#include <stdint.h>
#include <list>
#include <utility>
#include <stdio.h>

#define DEBUG_ON

namespace py {
namespace gc {


/*
    USAGE NOTES:

    any garbage collected class is expected to implement 
    namespace py {
        namespace gc {
            void for_each_child(gc_ptr<your type>&, auto&& visitor);
        }
    }

    where auto&& visitor will be a callable that should be applied to all of the 
    children of gc_ptr<your type> (each child being another gc_ptr that your type)
    is holding a reference to

    in this way the garbage collector is able to traverse the dependency tree 
    created by your type's 

    to garbage collect you are then expected to do 
        auto mark_visitor = heap.get_mark_visitor();
    and apply the mark visitor to all values you want to keep
        my_gc_ptr.gc_visit(mark_visitor);
    and finally you should clean up with 
        heap.sweep_marked_objects();
*/

// forward declare the templated class for gc_heap
template<class T>
class gc_heap;

/*
    a template class representing a garbage collected pointer type,
    note that any object that is not visited on a gc pass will be collected
*/
template<class T>
class gc_ptr {
private:
    static constexpr uint8_t FLAG_MARKED = 1;
    static constexpr uint8_t FLAG_DELETED = 1 << 1;

    struct gc_heap_object {
        uint8_t flags = 0;
        gc_heap<T>* owner;
        typename std::list<gc_heap_object>::iterator gc_heap_itr;
        T real;
    };
    
    gc_heap_object* heap_obj = nullptr;

    gc_ptr<T>(gc_heap_object* heap_obj) : heap_obj(heap_obj) { };
public:
    gc_ptr<T>(const gc_ptr<T>& other) : gc_ptr<T>(other.heap_obj) {
    }

    T* operator->() {
        return &(heap_obj->real);
    }

    T& operator*() {
        return heap_obj->real;
    }

    bool operator=(const gc_ptr<T>& other) {
        return this->heap_obj == other.heap_obj;
    }

    gc_ptr<T>& operator=(gc_ptr<T>& other) {
        this->heap_obj = other.heap_obj;
        return *this;
    }

    /*
        note gc_visit expects the for_each_child method to be implemented
        for the type T of the pointer, we will then use that method to iterate
        over the children of the class and check if they are still in
    */
    template<class VisitorType>
    void gc_visit(VisitorType& visitor) {
        if (!(heap_obj->flags & FLAG_MARKED)) {
            visitor(*this);
            for_each_child(*this, visitor);
        }
    }

    friend class gc_heap<T>;
};

/*
    a heap from which objects of type T may be allocated,
    it keeps track of the memory of all the objects and allows us to free them
    over time.
    NOTE: keeping references to the objects in the heap is forbidden, as their
    memory locations may be changed over time :)
    we intend to use a genrational garbage collector down the line
*/
template<class T>
class gc_heap {
private:
    using heap_object_type = typename gc_ptr<T>::gc_heap_object;

    std::list<heap_object_type> heap_objects;
public:
    template < typename... Args> 
    gc_ptr<T> make (Args&&... args) {
        heap_object_type obj;
        obj.owner = this;
        obj.real = T(std::forward<Args>(args)...);

        heap_objects.push_front(std::move(obj));
        obj.gc_heap_itr = heap_objects.begin();
        
        return &(this->heap_objects.front());
    }

    // NOTE: in debug mode especially, this function becomes incredibly expensive
    // i.e. O(n)
    // the intention is that this function should be used for debug only
    int heap_size() {
        return this->heap_objects.size();
    }
    
    void sweep_marked_objects() {
        for (auto itr = this->heap_objects.begin(); itr != this->heap_objects.end();) {
            auto &obj = *itr;
            if (!(obj.flags & gc_ptr<T>::FLAG_MARKED)) {
                heap_objects.erase(itr++);
            } else {
                (*itr).flags &= ~(gc_ptr<T>::FLAG_MARKED);
                ++itr;
            }
        }
    }

    // a visitor that we use to mark objects for retention (i.e. that they should)
    // not be deleted!!!
    const auto get_mark_visitor() const {
        return [] (gc_ptr<T>& val) {
           val.heap_obj->flags |= gc_ptr<T>::FLAG_MARKED;
        };
    }
};

}
}

// undefine any utility macro's created in this file
#undef DEBUG_CHECK_DELETED

#endif