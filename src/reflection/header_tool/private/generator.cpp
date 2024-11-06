#include "generator.hpp"

#include "header_parser.hpp"

#include <filesystem>
#include <iostream>
#include <random>

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
    header.write_line("#include \"class.hpp\"");
    header.new_line(3);

    for (const auto& gen_class : parser->get_classes())
    {
        header.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", gen_class.class_name));
        header.indent();
        {
            header.new_line(1);
            header.write_line(std::format("class {}; // forward declaration", gen_class.class_name));
            header.write_line(std::format("#define _REFLECTION_BODY_RUID_{}_LINE_{} REFL_DECLARE_CLASS({}); // forward declaration", global_refl_uid, gen_class.implementation_line, gen_class.class_name));
            header.write_line(std::format("REFL_DECLARE_TYPENAME({}); // declare type name for {}", gen_class.class_name, gen_class.class_name));
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
    source.new_line(3);

    for (const auto& gen_class : parser->get_classes())
    {
        source.write_line(std::format("/* ##############################  Reflection for {}  ############################## */", gen_class.class_name));
        source.indent();
        {
            source.new_line(1);
            source.write_line(std::format("Reflection::Class* _Static_Item_Class_{} = nullptr; // static class reference", gen_class.class_name));
            source.write_line(std::format("const Reflection::Class* {}::static_class() {{ return Reflection::Class::get<{}>(); }}", gen_class.class_name, gen_class.class_name));
            source.write_line(std::format("const Reflection::Class* {}::get_class() const {{ return Reflection::Class::get<{}>(); }}", gen_class.class_name, gen_class.class_name));

            // Populate class definition
            source.new_line(1);
            source.write_line(std::format("void _Refl_Register_Function_{}() {{ // Builder function", gen_class.class_name));
            source.indent();
            {
                source.write_line(std::format("_Static_Item_Class_{} = REFL_REGISTER_CLASS({});", gen_class.class_name, gen_class.class_name));
            }
            source.unindent();
            source.write_line("}");
            source.new_line(1);

            source.new_line(1);
            source.write_line(std::format("struct _Static_Item_Builder_{} {{ // Builder for {}", gen_class.class_name, gen_class.class_name));
            source.indent();
            {
                source.write_line(std::format("_Static_Item_Builder_{}() {{", gen_class.class_name));
                source.indent();
                {
                    source.write_line(std::format("_Refl_Register_Function_{}();", gen_class.class_name));
                }
                source.unindent();
                source.write_line("}");
            }
            source.unindent();
            source.write_line("};");
            source.write_line(std::format("_Static_Item_Builder_{} _Static_Item_Builder_{}_Var; //  Register {} on execution", gen_class.class_name, gen_class.class_name, gen_class.class_name));

            source.new_line(2);
        }
        source.unindent();

    }
}