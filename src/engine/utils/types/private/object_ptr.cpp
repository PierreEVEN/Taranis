#include "object_ptr.hpp"

#include "logger.hpp"
#include "object_allocator.hpp"

void IObject::destroy()
{
    if (*this)
    {
        // avoid double call when calling delete operator
        void* ptr       = allocation->ptr;
        allocation->ptr = nullptr;

        // Class destructor
        if (allocation->object_class)
        {
            allocation->object_class->placement_delete(ptr);
        }
        // Generated destructor
        else if (allocation->destructor)
        {
            allocation->destructor->destroy();
            delete allocation->destructor;
            allocation->destructor = nullptr;
        }
        else
            LOG_FATAL("Not destructor available for object")

        // Free using custom allocator
        if (allocation->allocator)
        {
            allocation->allocator->free(allocation->object_class, ptr);
            allocation->allocator = nullptr;
        }
        else
            std::free(ptr);

        allocation->object_class = nullptr;

        // Once smart pointer is no longer in use
        if (allocation->ptr_count == 0 && allocation->ref_count == 0)
            free();
    }
}