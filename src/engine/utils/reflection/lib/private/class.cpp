#include "class.hpp"

#include "property.hpp"

#include <iostream>
#include <ankerl/unordered_dense.h>

namespace Reflection
{
ankerl::unordered_dense::map<TypeId, Class*>*                                              Class::classes                              = nullptr;
ankerl::unordered_dense::map<TypeId, std::vector<Class*>>*                                 Class::class_waiting_parent_registration    = nullptr;
ankerl::unordered_dense::map<TypeId, std::vector<Class::PropertyWaitingTypeRegistration>>* Class::properties_waiting_type_registration = nullptr;

ankerl::unordered_dense::map<TypeId, std::vector<Class*>>& Class::get_class_waiting_type_registration()
{
    if (!class_waiting_parent_registration)
        class_waiting_parent_registration = new ankerl::unordered_dense::map<TypeId, std::vector<Class*>>();
    return *class_waiting_parent_registration;
}

ankerl::unordered_dense::map<TypeId, Class*>& Class::get_classes_internal()
{
    if (!classes)
        classes = new ankerl::unordered_dense::map<TypeId, Class*>();
    return *classes;
}

ankerl::unordered_dense::map<TypeId, std::vector<Class::PropertyWaitingTypeRegistration>>& Class::get_properties_waiting_type_registration()
{
    if (!properties_waiting_type_registration)
        properties_waiting_type_registration = new ankerl::unordered_dense::map<TypeId, std::vector<PropertyWaitingTypeRegistration>>();
    return *properties_waiting_type_registration;
}

Class* Class::get(const char* type_name)
{
    return get(make_type_id(type_name));
}

Class* Class::get(const TypeId& type_id)
{
    auto&      classes_ref = get_classes_internal();
    const auto found       = classes_ref.find(type_id);
    if (found != classes_ref.end())
        return found->second;
    return nullptr;
}

void Class::add_parent(const TypeId& parent)
{
    if (Class* FoundClass = get(parent))
        parents.push_back(FoundClass);
    else
        get_class_waiting_type_registration().insert_or_assign(parent, std::vector<Class*>{}).first->second.push_back(this);
}

void Class::register_property(const std::string& property_name, TypeId property_type_id, size_t offset)
{
    if (const Type* type = get_type(property_type_id))
    {
        if (!properties.emplace(property_name, new Property(property_name, type, offset)).second)
        {
            std::cerr << "Failed to register class property '" << property_name << "' for " << name() << "\n";
            exit(-1);
        }
    }
    else
    {
        std::cout << "TODO : handle class waiting for registration \n";
        get_properties_waiting_type_registration().insert_or_assign(property_type_id, std::vector<PropertyWaitingTypeRegistration>()).first->second.emplace_back(
            PropertyWaitingTypeRegistration{this, property_name, offset});
    }
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
    if (auto found = get_class_waiting_type_registration().find(inClass->id()); found != get_class_waiting_type_registration().end())
    {
        for (const auto& cl : found->second)
            cl->on_register_parent_class(inClass);
        get_class_waiting_type_registration().erase(inClass->id());
    }

    if (!get_classes_internal().emplace(inClass->id(), inClass).second)
    {
        std::cerr << "Failed to register class " << inClass->name() << "\n";
        exit(-1);
    }
}
} // namespace Reflection