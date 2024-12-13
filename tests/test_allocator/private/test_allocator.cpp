#include "logger.hpp"
#include "object_allocator.hpp"
#include "test_refl_class.hpp"

#include <filesystem>

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    ContiguousObjectAllocator alloc;

    ankerl::unordered_dense::map<TObjectPtr<TestReflectClass>, int> objects;

    for (int i = 0; i < 10000; ++i)
    {
        TObjectPtr<TestReflectClass> object(alloc.allocate(TestReflectClass::static_class()));
        object->identifier = i;
        assert(!objects.contains(object));
        objects.emplace(object, i);
    }

    for (const auto& obj : objects)
    {
        (void)obj;
        assert(obj.second == obj.first->identifier);
    }

    for (int i = 0; i < 9000; ++i)
    {
        int  n  = rand() % objects.size() - 1;
        auto it = objects.begin();
        for (; n > 0; --n, ++it)
            assert(it->second == it->first->identifier);
        objects.erase(it);
    }

    for (int i = 0; i < 10000; ++i)
    {
        TObjectPtr<TestReflectClass> object(alloc.allocate(TestReflectClass::static_class()));
        object->identifier = i;
        assert(!objects.contains(object));
        objects.emplace(object, i);
    }

    for (int i = 0; i < 9000; ++i)
    {
        int  n  = rand() % objects.size() - 1;
        auto it = objects.begin();
        for (; n > 0; --n, ++it)
            assert(it->second == it->first->identifier);
        objects.erase(it);
    }

    std::vector<TObjectPtr<TestReflectClass>> objects_A;
    for (const auto& object : objects)
        objects_A.emplace_back(object.first);

    objects.clear();

    auto objects_B = objects_A;

    std::vector<TObjectRef<TestReflectClass>> refs_A;
    for (const auto& object : objects)
        refs_A.emplace_back(object.first);

    objects_A.clear();

    for (int i = 0; i < 100; ++i)
        objects_B[rand() % (objects_B.size() - 1)].destroy();

    for (auto& Obj : objects_B)
        Obj.destroy();

    for (const auto& obj : refs_A)
    {
        (void)obj;
        assert(!obj);
    }
    for (const auto& obj : objects_B)
    {
        (void)obj;
        assert(!obj);
    }

    refs_A.clear();
    objects_B.clear();

    return 0;
}