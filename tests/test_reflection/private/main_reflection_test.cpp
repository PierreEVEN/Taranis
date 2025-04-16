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
    void serialize(Reflection::Archive& archive, void* alloc) override
    {
        Serializer* serializer = Serializer::get(Reflection::Type::make_type_id<T>());
        if (!serializer)
        {
            std::cerr << "There is no serializer for type " << Reflection::StaticTypeInfos<T>::name << "\n";
            return;
        }

        std::vector<T>& data   = *static_cast<std::vector<T>*>(alloc);
        size_t          length = data.size();
        archive <=> length;
        if (length == 0)
            return;
        if (archive.is_reading())
        {
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
    Reflection::Serializer::register_serializer<std::string, StringSerializer>();
    Reflection::Serializer::register_serializer<std::vector<float>, VectorSerializer<float>>();
    Reflection::Serializer::register_serializer<std::vector<std::vector<float>>, VectorSerializer<std::vector<float>>>();

    Reflection::Serializer::register_serializer<MyTestClass, Reflection::ClassSerializer<MyTestClass>>();
    Reflection::Serializer::register_serializer<TestChild, Reflection::ClassSerializer<TestChild>>();

    std::ifstream test_input("./saved/assets/test.asset", std::ios::binary);

    std::filesystem::create_directories("./saved/assets/");
    {
        MyTestClass test_instance;
        float       test_prop = 5;

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

    test_serializer();
}