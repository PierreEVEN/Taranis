#include "generator.hpp"

#include "header_parser.hpp"

#include <filesystem>
#include <iostream>
#include <random>
#include <format>

static size_t global_refl_uid = rand();

struct RandInit
{
    RandInit()
    {
        std::random_device                    rd;
        std::mt19937_64                       eng(rd());
        std::uniform_int_distribution<size_t> random;
        global_refl_uid = random(eng) / 2;
    }
};

static RandInit _rnd_init;

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

static void unpack_template_args(Generator::Writer& source, const TypeDefinition& type)
{
    return;
    std::string args;
    for (size_t i = 0; i < type.get_template_args().size(); ++i)
    {
        auto& arg = type.get_template_args()[i];
        unpack_template_args(source, arg);
        args += i >= type.get_template_args().size() - 1 ? arg.full_name_string() : arg.full_name_string() + ", ";
    }

    if (!args.empty())
        source.write_line(std::format("Reflection::Type::register_type_template<{}, {}>();", type.full_name_string(), args));
}

static std::string make_type_instance(const TypeDefinition& type)
{
    std::string base = std::format("Reflection::TypeInstance::create<{}>()", type.full_name_string());
    if (type.is_const)
        base += ".set_const()";
    if (type.is_ref)
        base += ".set_ref()";
    return base;
}

void Generator::generate(size_t                       timestamp,
                         const std::filesystem::path& source_path,
                         const std::filesystem::path& header_path,
                         const std::filesystem::path& base_header_path,
                         const std::filesystem::path& generated_header_include_path) const
{
    create_directories(source_path.parent_path());
    create_directories(header_path.parent_path());

    global_refl_uid++;

    std::string include_guard_name = header_path.filename().replace_extension("").replace_extension("").string() + "_gen_hpp";

    Writer header(header_path);
    header.write_line(std::format("/// VERSION : {}", timestamp));
    header.new_line();
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

    for (const auto& gen_enums : parser->get_enums())
    {
        auto enum_name = gen_enums.second.enum_path();
        header.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", enum_name));
        header.indent();
        {
            header.new_line(1);
            if (!gen_enums.second.context.namespace_stack.empty())
            {
                header.write_line(std::format("namespace {} {{", gen_enums.second.namespace_path()));
                header.indent();
            }
            header.write_line(std::format("enum {}{} : {}; // forward declaration", gen_enums.second.is_scoped() ? "class " : "", gen_enums.first, gen_enums.second.get_type()));
            if (!gen_enums.second.context.namespace_stack.empty())
            {
                header.unindent();
                header.write_line("}");
            }
            header.write_line(std::format("REFL_DECLARE_ENUM_TYPENAME({}); // declare type name for {}", enum_name, enum_name));

            if (gen_enums.second.is_enum_flag())
            {
                header.write_line(std::format("inline {} operator&({} a, {} b) {{ return static_cast<{}>(static_cast<size_t>(a) & static_cast<size_t>(b)); }}", enum_name, enum_name, enum_name, enum_name));
                header.write_line(std::format("inline {} operator|({} a, {} b) {{ return static_cast<{}>(static_cast<size_t>(a) | static_cast<size_t>(b)); }}", enum_name, enum_name, enum_name, enum_name));
                header.write_line(std::format("inline {} operator&=({}& a, {} b) {{ a = static_cast<{}>(static_cast<size_t>(a) & static_cast<size_t>(b)); return a; }}", enum_name, enum_name, enum_name, enum_name));
                header.write_line(std::format("inline {} operator|=({}& a, {} b) {{ a = static_cast<{}>(static_cast<size_t>(a) | static_cast<size_t>(b)); return a; }}", enum_name, enum_name, enum_name, enum_name));
            }
            header.new_line(2);
        }
        header.unindent();
    }
    for (const auto& gen_class : parser->get_classes())
    {
        std::string class_name = gen_class.second.class_path();

        header.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", class_name));
        header.indent();
        {
            header.new_line(1);
            if (!gen_class.second.context.namespace_stack.empty())
            {
                header.write_line(std::format("namespace {} {{", gen_class.second.namespace_path()));
                header.indent();
            }
            header.write_line(std::format("class {}; // forward declaration", gen_class.second.class_name()));
            if (!gen_class.second.context.namespace_stack.empty())
            {
                header.unindent();
                header.write_line("}");
            }
            header.write_line(
                std::format("#define _REFLECTION_BODY_RUID_{}_LINE_{} REFL_DECLARE_CLASS({}); // class body content", global_refl_uid, gen_class.second.implementation_line, gen_class.second.sanitized_class_path()));
            header.write_line(std::format("REFL_DECLARE_CLASS_TYPENAME({}); // declare type name for {}", class_name, class_name));
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

    for (const auto& gen_enum : parser->get_enums())
    {
        auto enum_name = gen_enum.second.enum_path();
        source.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", enum_name));
        source.indent();
        {
            // Populate enum definition
            source.new_line(1);
            source.write_line(std::format("void _Refl_Register_Function_{}() {{ // Builder function", gen_enum.second.sanitized_enum_path()));
            source.indent();
            {
                source.write_line(std::format("Reflection::Enum* _Static_Item_Enum_{} = Reflection::Enum::register_enum<{}>();", gen_enum.second.sanitized_enum_path(), enum_name));

                if (gen_enum.second.get_fields().empty())
                    source.write_line(std::format("(void)_Static_Item_Enum_{};", gen_enum.second.sanitized_enum_path()));

                for (const auto& field : gen_enum.second.get_fields())
                    source.write_line(std::format("_Static_Item_Enum_{}->register_field(\"{}\");", gen_enum.second.sanitized_enum_path(), field));
            }
            source.unindent();
            source.write_line("}");
            source.new_line(2);

            source.write_line(std::format("struct _Static_Item_Builder_{} {{ // Builder for {}", gen_enum.second.sanitized_enum_path(), enum_name));
            source.indent();
            {
                source.write_line(std::format("_Static_Item_Builder_{}() {{", gen_enum.second.sanitized_enum_path()));
                source.indent();
                {
                    source.write_line(std::format("_Refl_Register_Function_{}();", gen_enum.second.sanitized_enum_path()));
                }
                source.unindent();
                source.write_line("}");
            }
            source.unindent();
            source.write_line("};");
            source.write_line(std::format("_Static_Item_Builder_{} _Static_Item_Builder_{}_Var; //  Register {} on execution", gen_enum.second.sanitized_enum_path(), gen_enum.second.sanitized_enum_path(), enum_name));

            source.new_line(2);
        }
        source.unindent();
    }

    for (const auto& gen_class : parser->get_classes())
    {
        std::string class_name = gen_class.second.class_path();

        source.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", class_name));
        source.indent();
        {
            source.new_line(1);
            source.write_line(std::format("Reflection::Class* _Static_Item_Class_{} = nullptr; // static class reference", gen_class.second.sanitized_class_path()));
            source.write_line(std::format("const Reflection::Class* {}::static_class() {{ return Reflection::Class::get<{}>(); }}", class_name, class_name));
            source.write_line(std::format("const Reflection::Class* {}::get_class() const {{ return Reflection::Class::get<{}>(); }}", class_name, class_name));

            // Populate class definition
            source.new_line(1);
            source.write_line(std::format("void _Refl_Register_Function_{}() {{ // Builder function", gen_class.second.sanitized_class_path()));
            source.indent();
            {
                source.write_line(std::format("_Static_Item_Class_{} = Reflection::Class::register_class<{}>();", gen_class.second.sanitized_class_path(), class_name));
                for (const auto& parent : gen_class.second.get_parent_paths())
                {
                    source.write_line(std::format("_Static_Item_Class_{}->add_parent(Reflection::TypeId::create<{}>());", gen_class.second.sanitized_class_path(), parent));
                    source.write_line(std::format("_Static_Item_Class_{}->add_cast_function<{},{}>();", gen_class.second.sanitized_class_path(), class_name, parent));
                }

                for (const auto& property : gen_class.second.properties())
                {
                    unpack_template_args(source, property.second);
                    source.write_line(std::format(
                        "_Static_Item_Class_{}->register_property("
                        "\"{}\", "
                        "offsetof({}, {}), "
                        "{});",
                        gen_class.second.sanitized_class_path(),
                        property.first,
                        class_name, property.first,
                        make_type_instance(property.second)));
                }
            }
            source.unindent();
            source.write_line("}");
            source.new_line(1);

            source.new_line(1);
            source.write_line(std::format("struct _Static_Item_Builder_{} {{ // Builder for {}", gen_class.second.sanitized_class_path(), class_name));
            source.indent();
            {
                source.write_line(std::format("_Static_Item_Builder_{}() {{", gen_class.second.sanitized_class_path()));
                source.indent();
                {
                    source.write_line(std::format("_Refl_Register_Function_{}();", gen_class.second.sanitized_class_path()));
                }
                source.unindent();
                source.write_line("}");
            }
            source.unindent();
            source.write_line("};");
            source.write_line(
                std::format("_Static_Item_Builder_{} _Static_Item_Builder_{}_Var; //  Register {} on execution", gen_class.second.sanitized_class_path(), gen_class.second.sanitized_class_path(), class_name));

            source.new_line(2);
        }
        source.unindent();
    }
}