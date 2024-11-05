#include "class.hpp"

#include <unordered_map>


namespace Reflection
{
static std::unordered_map<size_t, Class*>* classes = nullptr;

std::unordered_map<size_t, Class*>& get_classes()
{
    if (!classes)
        classes = new std::unordered_map<size_t, Class*>();
    return *classes;
}


Class* Class::get(const std::string& type_name)
{
    auto&      classes_ref = get_classes();
    const auto found       = classes_ref.find(std::hash<std::string>{}(type_name));
    if (found != classes_ref.end())
        return found->second;
    return nullptr;
}

void Class::RegisterClass_Internal(Class* inClass)
{
    get_classes().emplace(inClass->get_id(), inClass);
}
}