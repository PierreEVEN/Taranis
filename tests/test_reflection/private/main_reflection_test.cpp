#include "test_class.hpp"
#include "logger.hpp"
#include "property.hpp"

Reflection::NativeTypeRecorder native_type_recorder;

static void test_serializer()
{
    Reflection::Serializer::register_serializer<uint8_t, Reflection::RawSerializer<uint8_t>>();
    Reflection::Serializer::register_serializer<uint16_t, Reflection::RawSerializer<uint16_t>>();
    Reflection::Serializer::register_serializer<uint32_t, Reflection::RawSerializer<uint32_t>>();
    Reflection::Serializer::register_serializer<uint64_t, Reflection::RawSerializer<uint64_t>>();
    Reflection::Serializer::register_serializer<int8_t, Reflection::RawSerializer<int8_t>>();
    Reflection::Serializer::register_serializer<int16_t, Reflection::RawSerializer<int16_t>>();
    Reflection::Serializer::register_serializer<int32_t, Reflection::RawSerializer<int32_t>>();
    Reflection::Serializer::register_serializer<int64_t, Reflection::RawSerializer<int64_t>>();
    Reflection::Serializer::register_serializer<bool, Reflection::RawSerializer<bool>>();
    Reflection::Serializer::register_serializer<float, Reflection::RawSerializer<float>>();
    Reflection::Serializer::register_serializer<double, Reflection::RawSerializer<double>>();

    Reflection::Serializer::register_serializer<MyTestClass, Reflection::ClassSerializer<MyTestClass>>();

    MyTestClass test_instance;

    Reflection::Archive out_archive;

    float test_prop = 5;
    out_archive <=> test_prop;

    out_archive <=> test_instance;
}


int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    //native_type_recorder = new Reflection::NativeTypeRecorder();

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
            LOG_INFO("\t\t- {} {} ({}+{}b)", property.second.get_type_instance().display(), property.second.get_name(), property.second.get_offset(), property.second.get_type_instance().stride());
    }

    MyTestClass test_class;
    LOG_INFO("Test instance class is {} (Static class is {})", test_class.get_class()->name(), MyTestClass::static_class()->name());
}