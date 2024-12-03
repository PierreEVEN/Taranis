#include "class.hpp"

#include <iostream>
#include <ankerl/unordered_dense.h>

namespace Reflection
{
static ankerl::unordered_dense::map<size_t, Class*>* classes = nullptr;

static ankerl::unordered_dense::map<std::string, std::vector<Class*>>* class_waiting_type_registration = nullptr;

ankerl::unordered_dense::map<std::string, std::vector<Class*>>& get_classes_wait_registration()
{
    if (!class_waiting_type_registration)
        class_waiting_type_registration = new ankerl::unordered_dense::map<std::string, std::vector<Class*>>();
    return *class_waiting_type_registration;
}

ankerl::unordered_dense::map<size_t, Class*>& get_classes()
{
    if (!classes)
        classes = new ankerl::unordered_dense::map<size_t, Class*>();
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

void Class::add_parent(const std::string& parent)
{
    if (Class* FoundClass = get(parent))
        parents.push_back(FoundClass);
    else
        get_classes_wait_registration().emplace(parent, std::vector{this}).first->second.push_back(this);
}

bool Class::is_base_of(const Class* base, const Class* t)
{
    if (!t)
        return false;

    if (base == t)
        return true;

    for (const auto& parent : t->parents)
        if (is_base_of(base, parent))
            return true;

    return false;
}

void Class::on_register_parent_class(Class* new_class)
{
    parents.push_back(new_class);
}

void Class::register_class_internal(Class* inClass)
{
    if (auto found = get_classes_wait_registration().find(inClass->name()); found != get_classes_wait_registration().end())
    {
        for (const auto& cl : found->second)
            cl->on_register_parent_class(inClass);
        get_classes_wait_registration().erase(inClass->name());
    }

    if (!get_classes().emplace(inClass->get_id(), inClass).second)
    {
        std::cerr << "Failed to register class " << inClass->name() << "\n";
        exit(-1);
    }
}
} // namespace Reflection