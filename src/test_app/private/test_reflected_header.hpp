#pragma once

#include "logger.hpp"
#include "macros.hpp"
#include "test_reflected_header.gen.hpp"

class ParentA
{
    REFLECT_BODY();



    int a = 1;

    virtual void my_func()
    {
    }

};

class ParentB
{
    REFLECT_BODY();
    int b = 2;

    virtual void my_func2()
    {
    }
};

class TestChildA : public ParentA, public ParentB
{
    REFLECT_BODY();
    int c = 3;

    virtual void my_func() override
    {
    }

    virtual void my_func2() override
    {
    }
};


class TestChildChild : public TestChildA
{
    REFLECT_BODY();
    int d = 4;

    virtual void my_func() override
    {
    }

    virtual void my_func2() override
    {
    }
};


inline void run_test()
{


    ParentB* child_a = new ParentB();
    LOG_WARNING("Base : {:x} (void{:x})", (size_t) reinterpret_cast<void*>(child_a), (size_t)(void*)child_a);

    child_a->cast<TestChildA>();

    ParentA* casted_ptr     = child_a->cast<ParentA>();
    ParentA* casted_ptr_tem = dynamic_cast<ParentA*>(child_a);
    LOG_WARNING("cast 1 : {:x} vs {:x}", (size_t)casted_ptr, (size_t)casted_ptr_tem);

    TestChildA* casted_ptr2     = child_a->cast<TestChildA>();
    TestChildA* casted_ptr2_tem = static_cast<TestChildA*>(child_a);
    LOG_WARNING("cast 2 : {:x} vs {:x}", (size_t)casted_ptr2, (size_t)casted_ptr2_tem);

    TestChildChild* casted_ptr3     = child_a->cast<TestChildChild>();
    TestChildChild* casted_ptr3_tem = static_cast<TestChildChild*>(child_a);
    LOG_WARNING("cast 3 : {:x} vs {:x}", (size_t)casted_ptr3, (size_t)casted_ptr3_tem);

    ParentB* casted_ptr4     = child_a->cast<ParentB>();
    ParentB* casted_ptr4_tem = dynamic_cast<ParentB*>(child_a);
    LOG_WARNING("cast 1 : {:x} vs {:x}", (size_t)casted_ptr4, (size_t)casted_ptr4_tem);
}