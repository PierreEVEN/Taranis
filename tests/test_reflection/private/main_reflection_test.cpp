#include "test_class.hpp"
#include "logger.hpp"
#include "property.hpp"

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    LOG_INFO("{} types registered :", Reflection::Type::get_types().size());
    for (const auto& type : Reflection::Type::get_types())
        LOG_INFO("\t- {} : {}b", type.second->name(), type.second->stride());

    LOG_INFO("{} classes registered :", Reflection::Class::get_classes().size());
    for (const auto& type : Reflection::Class::get_classes())
    {
        LOG_INFO("\t- {} : {}b", type.second->name(), type.second->stride());
        LOG_INFO("\t\tProperties :");
        for (const auto& property : type.second->get_properties())
            LOG_INFO("\t\t- {} {} ({}+{}b)", property.second->get_type()->name(), property.second->get_name(), property.second->get_offset(), property.second->get_type()->stride());
    }

    MyTestClass test_class;
    LOG_INFO("Test instance class is {} (Static class is {})", test_class.get_class()->name(), MyTestClass::static_class()->name());
}