#ifndef PYGC2_H
#define PYGC2_H

#define DEBUG_ON 15
#include <debug.hpp>
#include <list>
#include <functional>
#include <optional>
#include <memory>

#ifdef DEBUG_ON
#include <stdexcept>
#endif

namespace gc {

template<class T>
class gc_heap;

template<class T>
class gc_ptr {
private:
    struct shared_object {
        bool marked = false;
        size_t refcount = 0;
        // std::function<void()> cleanup_handler;
        std::list<shared_object>& heap_list;
        typename std::list<shared_object>::iterator heap_list_itr;

        std::optional<T> object;

        template < typename... Args> 
        shared_object(std::list<shared_object>& heap_list, Args&&... args) 
            : heap_list(heap_list), object(std::forward<Args>(args)...) {
        }

        ~shared_object() {
            DEBUG("DELETED SHARED_OBJECT AT ADDRESS %lu", (unsigned long long) this);
        }
    };
    
    shared_object& shared_obj;

    gc_ptr(shared_object& shared_obj) : shared_obj(shared_obj) {
        shared_obj.refcount++;
    }

public:
    gc_ptr(gc_ptr<T>& other) : shared_obj(other.shared_obj) {
        shared_obj.refcount++;
    }

    ~gc_ptr() {
        shared_obj.refcount--;
        if (shared_obj.refcount == 0) {
            shared_obj.heap_list.erase(shared_obj.heap_list_itr);
            // this used to just call cleanup_handler, which is still not looking like an entirely
            // terrible option :P 
        }
    }

    T* operator->() {
        return &(*shared_obj.object);
    }

    T& operator*() {
        return *shared_obj.object;
    }

    void mark() {
        shared_obj.marked = true;
#ifdef DEBUG_ON
        if (!(shared_obj.object)) {
            throw std::runtime_error("error, shared_obj.object is nontype");
        }
#endif
        for_each_child(*this);
    }

    friend class gc_heap<T>;
};

template<typename T>
class gc_heap {
private:
    using shared_object = typename gc_ptr<T>::shared_object;
    std::list<shared_object> heap_objects;

public:
    template < typename... Args> 
    gc_ptr<T> make(Args&&... args) {
        typename std::list<shared_object>::iterator obj_itr = heap_objects.emplace(
            heap_objects.begin(),
            heap_objects,
            std::forward<Args>(args)...
        );
        DEBUG("CONSTRUCTED INSTANCE OF SHARED_OBJECT, ADDRESS %lu", (unsigned long long) &(*obj_itr));

        (*obj_itr).heap_list_itr = obj_itr;

        return gc_ptr<T>(*obj_itr);
    }
    
    ~gc_heap() {
        DEBUG("DELETEING GC_HEAP<T>");
    }

    void sweep() {
        DEBUG("sweeping the heap");
        for (auto& obj : heap_objects) {
            if (obj.marked) {
                obj.marked = false;
            } else {
                // free the memory of the pointed object,
                // deallocate any pointers it may be holding thus breaking cycles
                // and letting the objects own destructors kick in to free the memory
                // that they are using!
                DEBUG("OBJECT %lu WAS NOT MARKED, SETTING ITS WRAPPED VALUE TO std::nullopt", &obj);
                DEBUG("\tTHIS WILL CAUSE ANY gc_ptr's IT IS HOLDING TO RELEASE THEIR HANDLES, AND LET NORMAL REFERENCE COUNTING TAKE OVER");
                if (!obj.object) {
                    DEBUG("\tUT OH. THE OBJECT WAS ALREADY SET TO NULLOPT, DID WE GC AGAIN TOO QUICKLY OR IS SOMETHING STILL HOLDING ON?");
                }
                obj.object = std::nullopt;
            }
        }
        DEBUG("done sweeping the heap.");
    }
};

}

#endif