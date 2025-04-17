#include "test_class.hpp"
#include "logger.hpp"
#include "property.hpp"
#include "serialization.hpp"
#include "stream.hpp"

Reflection::NativeTypeRecorder native_type_recorder;

class StringSerializer : public Reflection::Serializer
{
public:
    void serialize(Reflection::Archive& archive, void* alloc) override
    {
        std::string& data   = *static_cast<std::string*>(alloc);
        size_t       length = data.size();
        archive <=> length;
        if (length == 0)
            return;
        if (archive.is_reading())
            data.resize(length);
        archive.archive_raw(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data.c_str())), length);
    }
};

template <typename T> class VectorSerializer : public Reflection::Serializer
{
public:
    VectorSerializer(Reflection::TypeInstance in_type_instance) : type_instance(std::move(in_type_instance))
    {
    }

    void serialize(Reflection::Archive& archive, void* alloc) override
    {
        Serializer* serializer = get(type_instance);
        if (!serializer)
        {
            std::cerr << "There is no serializer for type " << type_instance.display() << "\n";
            return;
        }

        std::vector<T>& data   = *static_cast<std::vector<T>*>(alloc);
        size_t          length = data.size();
        archive <=> length;
        if (length == 0)
            return;
        if (archive.is_reading())
        {
            data.clear();
            data.reserve(length);
            for (size_t i = 0; i < length; ++i)
            {
                T item;
                serializer->serialize(archive, &item);
                data.emplace_back(std::move(item));
            }
        }
        else
            for (size_t i = 0; i < length; ++i)
                serializer->serialize(archive, &data[i]);
    }

private:
    Reflection::TypeInstance type_instance;
};

static void test_serializer()
{

    Reflection::Serializer::register_serializer<Reflection::RawSerializer<uint8_t>>(Reflection::Type::make_type_instance<uint8_t>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<uint16_t>>(Reflection::Type::make_type_instance<uint16_t>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<uint32_t>>(Reflection::Type::make_type_instance<uint32_t>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<uint64_t>>(Reflection::Type::make_type_instance<uint64_t>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<int8_t>>(Reflection::Type::make_type_instance<int8_t>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<int16_t>>(Reflection::Type::make_type_instance<int16_t>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<int32_t>>(Reflection::Type::make_type_instance<int32_t>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<int64_t>>(Reflection::Type::make_type_instance<int64_t>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<bool>>(Reflection::Type::make_type_instance<bool>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<float>>(Reflection::Type::make_type_instance<float>());
    Reflection::Serializer::register_serializer<Reflection::RawSerializer<double>>(Reflection::Type::make_type_instance<double>());
    Reflection::Serializer::register_serializer<StringSerializer>(Reflection::Type::make_type_instance<std::string>());
    Reflection::Serializer::register_serializer<VectorSerializer<float>>(
        Reflection::Type::make_type_instance<std::vector<float>>().set_template_specialization(Reflection::TypeSpecializationDescription({Reflection::Type::make_type_instance<float>()})),
        Reflection::Type::make_type_instance<float>()
        );

    Reflection::Serializer::register_serializer<VectorSerializer<double>>(
        Reflection::Type::make_type_instance<std::vector<double>>().set_template_specialization(Reflection::TypeSpecializationDescription({Reflection::Type::make_type_instance<double>()})),
        Reflection::Type::make_type_instance<double>());

    Reflection::Serializer::register_serializer<VectorSerializer<std::string>>(
        Reflection::Type::make_type_instance<std::vector<std::string>>().set_template_specialization(Reflection::TypeSpecializationDescription({Reflection::Type::make_type_instance<std::string>()})),
        Reflection::Type::make_type_instance<std::string>()
        );

    Reflection::Serializer::register_serializer<VectorSerializer<std::vector<double>>>(
        Reflection::Type::make_type_instance<std::vector<std::vector<double>>>().set_template_specialization(Reflection::TypeSpecializationDescription(
            {Reflection::Type::make_type_instance<std::vector<double>>().set_template_specialization(Reflection::TypeSpecializationDescription({Reflection::Type::make_type_instance<double>()}))})),
        Reflection::Type::make_type_instance<std::vector<double>>().set_template_specialization(Reflection::TypeSpecializationDescription({Reflection::Type::make_type_instance<double>()}))
        );

    Reflection::Serializer::register_serializer<Reflection::ClassSerializer<MyTestClass>>(Reflection::Type::make_type_instance<MyTestClass>());
    Reflection::Serializer::register_serializer<Reflection::ClassSerializer<TestChild>>(Reflection::Type::make_type_instance<TestChild>());

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

    //native_type_recorder = new Reflection::NativeTypeRecorder();

    LOG_INFO("{} types registered :", Reflection::Type::get_types().size());
    for (const auto& type : Reflection::Type::get_types())
    {
        if (type.second->is_template_type())
        {
            LOG_INFO("\t- {}<> : {}b", type.second->name(), type.second->stride());
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

    test_serializer();
}