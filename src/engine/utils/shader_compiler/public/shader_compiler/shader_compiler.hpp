#pragma once
#include "gfx_types/format.hpp"
#include "gfx_types/pipeline.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <ankerl/unordered_dense.h>
#include <vector>

namespace slang
{
struct IMetadata;
struct VariableLayoutReflection;
struct TypeReflection;
struct VariableReflection;
} // namespace slang

namespace slang
{
struct ISession;
struct IModule;
struct IGlobalSession;
} // namespace slang

namespace ShaderCompiler
{
class Compiler;
class ShaderParser;

struct StageInputOutputDescription
{
    uint32_t              location;
    uint32_t              offset;
    Eng::Gfx::ColorFormat format;
};

struct BindingDescription
{
    BindingDescription(std::string in_name, uint32_t in_binding, Eng::Gfx::EBindingType in_type, uint32_t in_array_elements)
        : name(std::move(in_name)), binding(in_binding), type(in_type), array_elements(in_array_elements)
    {
    }

    std::string            name;
    uint32_t               binding;
    Eng::Gfx::EBindingType type;
    uint32_t               array_elements = 0;
};

struct CompilationError
{
    CompilationError(std::string in_message, size_t in_line = 0, size_t in_column = 0) : message(std::move(in_message)), line(in_line), column(in_column)
    {
    }

    std::string message;
    size_t      line = 0, column = 0;
};

struct TypeReflection
{
    TypeReflection() = default;
    TypeReflection(slang::TypeReflection* slang_type);

    void push_variable(slang::VariableReflection* variable);

    static bool can_reflect(slang::TypeReflection* slang_var);

    std::size_t                                               total_stride = 0;
    ankerl::unordered_dense::map<std::string, TypeReflection> children;
};

struct StageData
{
    std::vector<uint8_t>                                                   compiled_module;
    std::vector<BindingDescription>                                        bindings;
    Eng::Gfx::EShaderStage                                                 stage;
    ankerl::unordered_dense::map<std::string, StageInputOutputDescription> inputs;
    uint32_t                                                               push_constant_size = 0;
};

struct CompilationResult
{
    CompilationResult& push_error(const CompilationError& error)
    {
        errors.emplace_back(error);
        return *this;
    }

    std::vector<CompilationError>                                   errors;
    ankerl::unordered_dense::map<Eng::Gfx::EShaderStage, StageData> stages;
};

class Session
{
public:
    CompilationResult compile(const std::string& render_pass, const Eng::Gfx::PermutationDescription& permutation) const;

    Eng::Gfx::PermutationDescription get_default_permutations_description() const
    {
        return {permutation_description};
    }

    ~Session();

    std::optional<std::filesystem::path> get_filesystem_path() const;

private:
    friend class Compiler;
    Session(Compiler* in_compiler, const std::filesystem::path& path);

    static std::optional<std::string> try_register_variable(slang::VariableLayoutReflection* variable, ankerl::unordered_dense::map<std::string, StageInputOutputDescription>& in_outs, slang::IMetadata* metadata);

    mutable std::mutex            session_lock;
    Compiler*                     compiler = nullptr;
    slang::IModule*               module   = nullptr;
    slang::ISession*              session  = nullptr;
    Eng::Gfx::PermutationGroup    permutation_description;
    std::vector<CompilationError> load_errors;
};

class Compiler
{
public:
    ~Compiler();

    static Compiler& get();

    std::shared_ptr<Session> create_session(const std::filesystem::path& path);

private:
    friend Session;
    std::mutex             global_session_lock;
    slang::IGlobalSession* global_session = nullptr;

    Compiler();
};
} // namespace ShaderCompiler