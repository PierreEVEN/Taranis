#include "enum.hpp"
#include "test_class.hpp"
#include "logger.hpp"
#include "property.hpp"
#include "serialization.hpp"
#include "stream.hpp"

template <typename T> class InPlacePtrSerializer : public Reflection::Serializer
{
public:
    void serialize(Reflection::Archive& archive, void* alloc) override
    {
        Reflection::TypeId class_ref_type = Reflection::TypeId::create<T>();
        Serializer*        serializer     = get(class_ref_type);
        if (!serializer)
        {
            std::cerr << "There is no serializer for type " << class_ref_type.name() << "\n";
            return;
        }

        T*&  data       = *static_cast<T**>(alloc);
        bool b_is_valid = data != nullptr;
        archive <=> b_is_valid;
        if (!b_is_valid)
        {
            data = nullptr;
            return;
        }
        if (archive.is_reading())
        {
            if (!data)
                data = new T();
            serializer->serialize(archive, data);
        }
        else
            serializer->serialize(archive, data);
    }
};


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

    // String serializers
    Reflection::Serializer::register_serializer<std::string, Reflection::StringSerializer>();

    // Vector serializers
    Reflection::Serializer::register_serializer<std::vector<float>, Reflection::VectorSerializer<float>>();
    Reflection::Serializer::register_serializer<std::vector<double>, Reflection::VectorSerializer<double>>();
    Reflection::Serializer::register_serializer<std::vector<std::string>, Reflection::VectorSerializer<std::string>>();
    Reflection::Serializer::register_serializer<std::vector<std::vector<double>>, Reflection::VectorSerializer<std::vector<double>>>();

    // Class Serializers
    Reflection::Serializer::register_serializer<MyTestClass, Reflection::ClassSerializer<MyTestClass>>();
    Reflection::Serializer::register_serializer<TestChild, Reflection::ClassSerializer<TestChild>>();

    // Ptr Serializers
    Reflection::Serializer::register_serializer<MyTestClass*, InPlacePtrSerializer<MyTestClass>>();

    std::ifstream test_input("./saved/assets/test.asset", std::ios::binary);

    std::filesystem::create_directories("./saved/assets/");
    {
        MyTestClass test_instance = MyTestClass::make_special();
        float       test_prop     = 5;

        Reflection::Archive out_archive = Reflection::Archive::create<Io::FileStream>(Io::Stream::Mode::Output, "./saved/assets/test.asset");
        out_archive <=> test_prop;
        out_archive <=> test_instance;
    }

    {
        MyTestClass test_instance;
        float       test_prop;

        Reflection::Archive in_archive = Reflection::Archive::create<Io::FileStream>(Io::Stream::Mode::Input, "./saved/assets/test.asset");
        in_archive <=> test_prop;
        in_archive <=> test_instance;

        Reflection::Archive out_archive = Reflection::Archive::create<Io::FileStream>(Io::Stream::Mode::Output, "./saved/assets/test_after.asset");
        out_archive <=> test_prop;
        out_archive <=> test_instance;
    }
}


int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    LOG_INFO("{} enums registered :", Reflection::Enum::get_enums().size());
    for (const auto& type : Reflection::Enum::get_enums())
    {
        LOG_INFO("\t- {} : {}b", type.second->name(), type.second->stride());
        for (const auto& field : type.second->fields())
            LOG_INFO("\t\t>{}", field);
    }

    LOG_INFO("{} types registered :", Reflection::Type::get_types().size());
    for (const auto& type : Reflection::Type::get_types())
    {
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

    test_serializer();
}