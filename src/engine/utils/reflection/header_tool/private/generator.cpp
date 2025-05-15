#include "generator.hpp"

#include "header_parser.hpp"

#include <filesystem>
#include <format>
#include <random>

static size_t random_init()
{
    std::random_device                    rd;
    std::mt19937_64                       eng(rd());
    std::uniform_int_distribution<size_t> random;
    return random(eng);
}

static size_t global_refl_uid = random_init();

Generator::Writer::Writer(const std::filesystem::path& file) : fs(file)
{
}

void Generator::Writer::write_line(const std::string& line)
{
    make_indent();
    fs << line << "\n";
}

void Generator::Writer::new_line(uint32_t num)
{
    for (uint32_t i = 0; i < num; ++i)
        fs << "\n";
}

void Generator::Writer::indent(uint32_t num)
{
    indentation += num;
}

void Generator::Writer::unindent(uint32_t num)
{
    if (num >= indentation)
        indentation = 0;
    else
        indentation -= num;
}

void Generator::Writer::make_indent()
{
    for (uint32_t i = 0; i < indentation; ++i)
        fs << "\t";
}

Generator::Generator(HeaderParser& in_parser) : parser(&in_parser)
{
}

static std::string make_type_instance(const Type& type)
{
    std::string base = std::format("Reflection::TypeInstance::create<{}>()", type.cpp_name());
    if (type.is_const())
        base += ".set_const()";
    if (type.is_ref())
        base += ".set_ref()";
    return base;
}

void Generator::generate(const std::filesystem::path& source_path, const std::filesystem::path& header_path, const std::filesystem::path& base_header_path,
                         const std::filesystem::path& generated_header_include_path) const
{
    create_directories(source_path.parent_path());
    create_directories(header_path.parent_path());

    global_refl_uid++;

    std::string include_guard_name = header_path.filename().replace_extension("").replace_extension("").string() + "_gen_hpp";

    Writer header(header_path);
    header.write_line("/**** GENERATED FILE BY REFLECTION TOOL, DO NOT MODIFY ****/");
    header.new_line();
    header.write_line("#undef _REFL_FILE_UNIQUE_ID_");
    header.write_line(std::format("#define _REFL_FILE_UNIQUE_ID_ RUID_{}", global_refl_uid));
    header.new_line(2);
    header.write_line(std::format("#ifndef _REFL_{}", include_guard_name));
    header.write_line(std::format("#define _REFL_{}", include_guard_name));
    header.new_line(1);
    header.write_line("#include <macros.hpp>");
    header.new_line(3);

    /*************** TEMPLATE TYPES DECLARATIONS ***************/
    for (const auto& gen_class : parser->get_classes())
    {
        for (const auto& property_kp : gen_class.second->get_properties())
        {
            std::string property_name = property_kp.first;
            const Type& property      = property_kp.second;
            // If not a template type
            if (!property.is_template())
                continue;

            header.write_line(std::format("#ifndef __DEF_REFL_DECLARE_TYPENAME_{}", property.sanitized_name()));
            header.indent();
            header.write_line(std::format("#define __DEF_REFL_DECLARE_TYPENAME_{}", property.sanitized_name()));

            if (property.name().has_namespace())
            {
                header.write_line(std::format("namespace {} {{", property.name().cpp_namespace()));
                header.indent();
            }
            size_t param_count = 1;
            if (property.name().short_name() == "vector")
                param_count = 2;
            std::string params;
            char chr = 'A';
            for (size_t i = 0; i < param_count; ++i)
                params += std::format("typename {},", chr++);

            header.write_line(std::format("template<{}> class {}; // forward declaration", params, property.name().short_name()));
            if (property.name().has_namespace())
            {
                header.unindent();
                header.write_line("}");
            }
            header.write_line(std::format("REFL_DECLARE_TYPENAME({}) // declare template type name for {}", property.cpp_name(), property.cpp_name()));
            header.unindent();
            header.write_line(std::format("#endif"));
        }
    }

    /*************** ENUMS HEADERS ***************/
    for (const auto& gen_enums_kp : parser->get_enums())
    {
        const auto& gen_enum      = gen_enums_kp.second;
        auto        enum_cpp_name = gen_enum.name().cpp_name();
        header.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", enum_cpp_name));
        header.indent();
        {
            header.new_line(1);
            if (gen_enum.name().has_namespace())
            {
                header.write_line(std::format("namespace {} {{", gen_enum.name().cpp_namespace()));
                header.indent();
            }
            if (gen_enum.get_type().empty())
                header.write_line(std::format("enum {}{}; // forward declaration", gen_enum.is_scoped() ? "class " : "", gen_enum.name().short_name()));
            else
                header.write_line(std::format("enum {}{} : {}; // forward declaration", gen_enum.is_scoped() ? "class " : "", gen_enum.name().short_name(), gen_enum.get_type()));
            if (gen_enum.name().has_namespace())
            {
                header.unindent();
                header.write_line("}");
            }
            header.write_line(std::format("REFL_DECLARE_ENUM_TYPENAME({}); // declare type name for {}", enum_cpp_name, enum_cpp_name));

            if (gen_enum.is_enum_flag())
            {
                header.write_line(
                    std::format("inline {} operator&({} a, {} b) {{ return static_cast<{}>(static_cast<size_t>(a) & static_cast<size_t>(b)); }}", enum_cpp_name, enum_cpp_name, enum_cpp_name, enum_cpp_name));
                header.write_line(
                    std::format("inline {} operator|({} a, {} b) {{ return static_cast<{}>(static_cast<size_t>(a) | static_cast<size_t>(b)); }}", enum_cpp_name, enum_cpp_name, enum_cpp_name, enum_cpp_name));
                header.write_line(
                    std::format("inline {} operator&=({}& a, {} b) {{ a = static_cast<{}>(static_cast<size_t>(a) & static_cast<size_t>(b)); return a; }}", enum_cpp_name, enum_cpp_name, enum_cpp_name, enum_cpp_name));
                header.write_line(
                    std::format("inline {} operator|=({}& a, {} b) {{ a = static_cast<{}>(static_cast<size_t>(a) | static_cast<size_t>(b)); return a; }}", enum_cpp_name, enum_cpp_name, enum_cpp_name, enum_cpp_name));
            }
            header.new_line(2);
        }
        header.unindent();
    }
    /*************** CLASS HEADERS ***************/
    for (const auto& class_kp : parser->get_classes())
    {
        const auto& gen_class      = class_kp.second;
        std::string cpp_name       = gen_class->name().cpp_name();
        std::string sanitized_name = gen_class->name().sanitized_name();

        header.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", cpp_name));
        header.indent();
        {
            header.new_line(1);

            header.write_line(std::format("void _Refl_Register_Function_{}(); // Forward declaration of builder function", sanitized_name));

            if (gen_class->name().has_namespace())
            {
                header.write_line(std::format("namespace {} {{", gen_class->name().cpp_namespace()));
                header.indent();
            }
            header.write_line(std::format("class {}; // forward declaration", gen_class->name().short_name()));
            if (gen_class->name().has_namespace())
            {
                header.unindent();
                header.write_line("}");
            }
            header.write_line(std::format("#define _REFLECTION_BODY_RUID_{}_LINE_{} REFL_DECLARE_CLASS({}, {}); // class body content", global_refl_uid, gen_class->get_implementation_line(), cpp_name, sanitized_name));

            header.write_line(std::format("#ifndef __DEF_REFL_DECLARE_TYPENAME_{}", sanitized_name));
            header.indent();
            header.write_line(std::format("#define __DEF_REFL_DECLARE_TYPENAME_{}", sanitized_name));
            header.write_line(std::format("REFL_DECLARE_CLASS_TYPENAME({}); // declare type name for {}", cpp_name, cpp_name));
            header.unindent();
            header.write_line(std::format("#endif"));
            header.new_line(2);
        }
        header.unindent();
    }

    header.new_line();
    header.write_line(std::format("#endif  // _REFL_{}", include_guard_name));

    Writer source(source_path);
    source.write_line("/**** GENERATED FILE BY REFLECTION TOOL, DO NOT MODIFY ****/");
    source.new_line();
    source.write_line(std::format("#include \"{}\"", generated_header_include_path.lexically_normal().string()));
    source.write_line(std::format("#include \"{}\"", base_header_path.lexically_normal().string()));
    source.write_line("#include <enum.hpp>");
    source.new_line(3);

    for (const auto& enum_kp : parser->get_enums())
    {
        const auto& gen_enum       = enum_kp.second;
        auto        cpp_name       = gen_enum.name().cpp_name();
        auto        sanitized_name = gen_enum.name().sanitized_name();
        source.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", cpp_name));
        source.indent();
        {
            // Populate enum definition
            source.new_line(1);
            source.write_line(std::format("void _Refl_Register_Function_{}() {{ // Builder function", sanitized_name));
            source.indent();
            {
                source.write_line(std::format("Reflection::Enum* _Static_Item_Enum_{} = Reflection::Enum::register_enum<{}>();", sanitized_name, cpp_name));

                if (gen_enum.get_fields().empty())
                    source.write_line(std::format("(void)_Static_Item_Enum_{};", sanitized_name));

                for (const auto& field : gen_enum.get_fields())
                    source.write_line(std::format("_Static_Item_Enum_{}->register_field(\"{}\");", sanitized_name, field));
            }
            source.unindent();
            source.write_line("}");
            source.new_line(2);

            source.write_line(std::format("struct _Static_Item_Builder_{} {{ // Builder for {}", sanitized_name, cpp_name));
            source.indent();
            {
                source.write_line(std::format("_Static_Item_Builder_{}() {{", sanitized_name));
                source.indent();
                {
                    source.write_line(std::format("_Refl_Register_Function_{}();", sanitized_name));
                }
                source.unindent();
                source.write_line("}");
            }
            source.unindent();
            source.write_line("};");
            source.write_line(std::format("_Static_Item_Builder_{} _Static_Item_Builder_{}_Var; //  Register {} on execution", sanitized_name, sanitized_name, cpp_name));

            source.new_line(2);
        }
        source.unindent();
    }

    for (const auto& class_kp : parser->get_classes())
    {
        const auto& gen_class      = class_kp.second;
        std::string cpp_name       = gen_class->name().cpp_name();
        std::string sanitized_name = gen_class->name().sanitized_name();

        source.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", cpp_name));
        source.indent();
        {
            source.new_line(1);
            source.write_line(std::format("Reflection::Class* _Static_Item_Class_{} = nullptr; // static class reference", sanitized_name));
            source.write_line(std::format("const Reflection::Class* {}::static_class() {{ return Reflection::Class::get<{}>(); }}", cpp_name, cpp_name));
            source.write_line(std::format("const Reflection::Class* {}::get_class() const {{ return Reflection::Class::get<{}>(); }}", cpp_name, cpp_name));

            // Populate class definition
            source.new_line(1);
            source.write_line(std::format("void _Refl_Register_Function_{}() {{ // Builder function", sanitized_name));
            source.indent();
            {
                source.write_line(std::format("_Static_Item_Class_{} = Reflection::Class::register_class<{}>();", sanitized_name, cpp_name));
                for (const auto& parent : gen_class->get_parents())
                {
                    source.write_line(std::format("_Static_Item_Class_{}->add_parent(Reflection::TypeId::create<{}>());", sanitized_name, parent.cpp_name()));
                    source.write_line(std::format("_Static_Item_Class_{}->add_cast_function<{},{}>();", sanitized_name, cpp_name, parent.cpp_name()));
                }

                for (const auto& property : gen_class->get_properties())
                {
                    source.write_line(std::format("_Static_Item_Class_{}->register_property("
                                                  "\"{}\", "
                                                  "offsetof({}, {}), "
                                                  "{});",
                                                  sanitized_name, property.first, cpp_name, property.first, make_type_instance(property.second)));
                }
            }
            source.unindent();
            source.write_line("}");
            source.new_line(1);

            source.new_line(1);
            source.write_line(std::format("struct _Static_Item_Builder_{} {{ // Builder for {}", sanitized_name, cpp_name));
            source.indent();
            {
                source.write_line(std::format("_Static_Item_Builder_{}() {{", sanitized_name));
                source.indent();
                {
                    source.write_line(std::format("_Refl_Register_Function_{}();", sanitized_name));
                }
                source.unindent();
                source.write_line("}");
            }
            source.unindent();
            source.write_line("};");
            source.write_line(std::format("_Static_Item_Builder_{} _Static_Item_Builder_{}_Var; //  Register {} on execution", sanitized_name, sanitized_name, cpp_name));

            source.new_line(2);
        }
        source.unindent();
    }
}