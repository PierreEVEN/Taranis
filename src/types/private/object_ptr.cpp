#include "object_ptr.hpp"
#include "object_allocator.hpp"


void IObject::destroy()
{
    if (*this)
    {
        internal_delete_ptr();
        if (allocation->allocator && allocation->object_class)
        {
            allocation->allocator->free(allocation->object_class, allocation->ptr);
        }
        else
        {
            std::free(allocation->ptr);
        }
        allocation->ptr          = nullptr;
        allocation->allocator    = nullptr;
        allocation->object_class = nullptr;
        if (allocation->ptr_count == 0 && allocation->ref_count == 0)
            free();
    }
}