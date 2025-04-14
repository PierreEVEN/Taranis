#include "test_class.hpp"
#include "logger.hpp"
#include "property.hpp"

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    LOG_INFO("{} types registered :", Reflection::Type::get_types().size());
    for (const auto& type : Reflection::Type::get_types())
    {
        if (type.second->is_template_type())
        {
            std::string templates = "available specializations : ";

            auto it = type.second->get_specializations().begin();
            while (it != type.second->get_specializations().end())
            {
                std::string args_str = "<";
                auto        arg      = it->second.get_args().begin();
                while (arg != it->second.get_args().end())
                {
                    args_str += (*arg)->name();
                    ++arg;
                    if (arg != it->second.get_args().end())
                        args_str += ", ";
                }
                templates += args_str + ">(" + std::to_string(it->second.stride()) + "b)";
                ++it;
                if (it != type.second->get_specializations().end())
                    templates += ", ";
            }
            LOG_INFO("\t- {} : {}b | {}", type.second->name(), type.second->stride(), templates);
        }
        else
            LOG_INFO("\t- {} : {}b", type.second->name(), type.second->stride());
    }

    LOG_INFO("{} classes registered :", Reflection::Class::get_classes().size());
    for (const auto& type : Reflection::Class::get_classes())
    {
        LOG_INFO("\t- {} : {}b", type.second->name(), type.second->stride());
        LOG_INFO("\t\tProperties :");
        for (const auto& property : type.second->get_properties())
            LOG_INFO("\t\t- {} {} ({}+{}b)", property.second->display_type(), property.second->get_name(), property.second->get_offset(), property.second->get_type()->stride());
    }

    MyTestClass test_class;
    LOG_INFO("Test instance class is {} (Static class is {})", test_class.get_class()->name(), MyTestClass::static_class()->name());
}