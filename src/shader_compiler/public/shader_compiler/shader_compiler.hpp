#pragma once
#include "gfx_types/format.hpp"
#include "gfx_types/pipeline.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace slang
{
struct TypeReflection;
struct VariableReflection;
}

namespace slang
{
struct ISession;
struct IModule;
struct IGlobalSession;
}

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
    BindingDescription(std::string in_name, uint32_t in_binding, Eng::Gfx::EBindingType in_type) : name(std::move(in_name)), binding(in_binding), type(in_type)
    {
    }

    std::string            name;
    uint32_t               binding;
    Eng::Gfx::EBindingType type;
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

    std::size_t                                     total_stride = 0;
    std::unordered_map<std::string, TypeReflection> children;
};

struct StageData
{
    std::vector<uint8_t>            compiled_module;
    std::vector<BindingDescription> bindings;
    Eng::Gfx::EShaderStage          stage;
    uint32_t                        push_constant_size = 0;
    TypeReflection                  inputs;
    TypeReflection                  outputs;
};

struct CompilationResult
{
    CompilationResult& push_error(const CompilationError& error)
    {
        errors.emplace_back(error);
        return *this;
    }

    std::vector<CompilationError>                         errors;
    std::unordered_map<Eng::Gfx::EShaderStage, StageData> stages;
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

private:
    friend class Compiler;
    Session(Compiler* in_compiler, const std::filesystem::path& path);

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

    slang::IGlobalSession* global_session = nullptr;

    Compiler();
};
}