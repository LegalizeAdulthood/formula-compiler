// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Formula.h>

#include <formula/Compiler.h>
#include <formula/Interpreter.h>

#include "descent.h"
#include "parser.h"

#include <cstdint>
#include <utility>

using namespace formula::ast;

namespace formula
{

namespace
{

class ParsedFormula : public Formula
{
public:
    explicit ParsedFormula(FormulaSectionsPtr ast);
    ~ParsedFormula() override = default;

    void set_value(std::string_view name, Complex value) override
    {
        m_state.symbols[std::string{name}] = value;
    }
    Complex get_value(std::string_view name) const override
    {
        if (auto it = m_state.symbols.find(std::string{name}); it != m_state.symbols.end())
        {
            return it->second;
        }
        return {};
    }
    const Expr &get_section(Section section) const override;

    Complex interpret(Section part) override;
    bool compile() override;
    Complex run(Section part) override;

private:
    using Function = double();

    CompileError init_code_holder(asmjit::CodeHolder &code);
    CompileError compile_part(asmjit::x86::Compiler &comp, Expr node, asmjit::Label &label);

    EmitterState m_state;
    FormulaSectionsPtr m_ast;
    Function *m_initialize{};
    Function *m_iterate{};
    Function *m_bailout{};
    asmjit::JitRuntime m_runtime;
    asmjit::FileLogger m_logger{stdout};
};

ParsedFormula::ParsedFormula(FormulaSectionsPtr ast) :
    m_ast(std::move(ast))
{
    m_state.symbols["e"] = {std::exp(1.0), 0.0};
    m_state.symbols["pi"] = {std::atan2(0.0, -1.0), 0.0};
    m_state.symbols["_result"] = {0.0, 0.0};
}

const Expr &ParsedFormula::get_section(Section section) const
{
    switch (section)
    {
    case Section::PER_IMAGE:
        return m_ast->per_image;
    case Section::BUILTIN:
        return m_ast->builtin;
    case Section::INITIALIZE:
        return m_ast->initialize;
    case Section::ITERATE:
        return m_ast->iterate;
    case Section::BAILOUT:
        return m_ast->bailout;
    case Section::PERTURB_INITIALIZE:
        return m_ast->perturb_initialize;
    case Section::PERTURB_ITERATE:
        return m_ast->perturb_iterate;
    case Section::DEFAULT:
        return m_ast->defaults;
    case Section::SWITCH:
        return m_ast->type_switch;

    default:
        throw std::runtime_error("Unknown section " + std::to_string(+section));
    }
}

Complex ParsedFormula::interpret(Section part)
{
    switch (part)
    {
    case Section::PER_IMAGE:
        return ast::interpret(m_ast->per_image, m_state.symbols);
    case Section::INITIALIZE:
        return ast::interpret(m_ast->initialize, m_state.symbols);
    case Section::ITERATE:
        return ast::interpret(m_ast->iterate, m_state.symbols);
    case Section::BAILOUT:
        return ast::interpret(m_ast->bailout, m_state.symbols);
    case Section::PERTURB_INITIALIZE:
        return ast::interpret(m_ast->perturb_initialize, m_state.symbols);
    case Section::PERTURB_ITERATE:
        return ast::interpret(m_ast->perturb_iterate, m_state.symbols);
    }
    throw std::runtime_error("Invalid part for interpreter");
}

CompileError ParsedFormula::init_code_holder(asmjit::CodeHolder &code)
{
    ASMJIT_CHECK(code.init(m_runtime.environment(), m_runtime.cpuFeatures()));
    code.setLogger(&m_logger);
    if (asmjit::Error err =
            code.newSection(&m_state.data.data, ".data", SIZE_MAX, asmjit::SectionFlags::kNone, sizeof(double), 0))
    {
        std::cerr << "Failed to create data section: " << asmjit::DebugUtils::errorAsString(err) << '\n';
        return err;
    }
    return {};
}

CompileError update_symbols(
    asmjit::x86::Compiler &comp, SymbolTable &symbols, SymbolBindings &bindings, asmjit::x86::Xmm result)
{
    asmjit::x86::Gp tmp{comp.newUIntPtr()};
    {
        double *_result{&symbols["_result"].re};
        auto dest = asmjit::x86::ptr(std::uintptr_t(_result));
        ASMJIT_CHECK(comp.movq(tmp, result));
        ASMJIT_CHECK(comp.mov(dest, tmp));
        dest = asmjit::x86::ptr(std::uintptr_t(_result + 1));
        ASMJIT_CHECK(comp.shufpd(result, result, 1)); // result = result.yx
        ASMJIT_CHECK(comp.movq(tmp, result));
        ASMJIT_CHECK(comp.mov(dest, tmp));
    }
    for (auto &[name, binding] : bindings)
    {
        auto src = asmjit::x86::ptr(binding.label);
        double *real{&symbols[name].re};
        auto dest = asmjit::x86::ptr(std::uintptr_t(real));
        ASMJIT_CHECK(comp.mov(tmp, src));
        ASMJIT_CHECK(comp.mov(dest, tmp));
        src = asmjit::x86::ptr(binding.label, sizeof(double));
        dest = asmjit::x86::ptr(std::uintptr_t(real + 1));
        ASMJIT_CHECK(comp.mov(tmp, src));
        ASMJIT_CHECK(comp.mov(dest, tmp));
    }
    return {};
}

CompileError ParsedFormula::compile_part(asmjit::x86::Compiler &comp, Expr node, asmjit::Label &label)
{
    label = comp.addFunc(asmjit::FuncSignature::build<double>())->label();
    asmjit::x86::Xmm result = comp.newXmmSd();
    if (const CompileError err = ast::compile(node, comp, m_state, result); err)
    {
        std::cerr << "Failed to compile AST\n" << asmjit::DebugUtils::errorAsString(err.value()) << '\n';
        ;
        return err;
    }
    if (const CompileError err = update_symbols(comp, m_state.symbols, m_state.data.symbols, result); err)
    {
        return err;
    }
    ASMJIT_CHECK(comp.ret(result));
    ASMJIT_CHECK(comp.endFunc());
    return {};
}

template <typename FunctionPtr>
FunctionPtr function_cast(const asmjit::CodeHolder &code, char *module, const asmjit::Label label)
{
    if (!label.isValid())
    {
        return nullptr;
    }
    const size_t offset{code.labelOffsetFromBase(label)};
    return reinterpret_cast<FunctionPtr>(module + offset);
}

CompileError emit_data_section(asmjit::x86::Compiler &comp, EmitterState &state)
{
    ASMJIT_CHECK(comp.section(state.data.data));
    for (auto &[name, binding] : state.data.symbols)
    {
        if (binding.bound)
        {
            continue;
        }
        binding.bound = true;
        ASMJIT_CHECK(comp.bind(binding.label));
        if (const auto it = state.symbols.find(name); it != state.symbols.end())
        {
            ASMJIT_CHECK(comp.embedDouble(it->second.re));
            ASMJIT_CHECK(comp.embedDouble(it->second.im));
        }
        else
        {
            throw std::runtime_error("Symbol not found: " + name);
        }
    }
    for (auto &[value, binding] : state.data.constants)
    {
        if (binding.bound)
        {
            continue;
        }
        binding.bound = true;
        ASMJIT_CHECK(comp.bind(binding.label));
        ASMJIT_CHECK(comp.embedDouble(value.re));
        ASMJIT_CHECK(comp.embedDouble(value.im));
    }
    return {};
}

bool ParsedFormula::compile()
{
    asmjit::CodeHolder code;
    if (const CompileError err = init_code_holder(code); err)
    {
        std::cerr << "Failed to initialize code holder:\n" << asmjit::DebugUtils::errorAsString(err.value()) << '\n';
        return false;
    }
    asmjit::x86::Compiler comp(&code);

    asmjit::Label init_label{};
    asmjit::Label iterate_label{};
    asmjit::Label bailout_label{};
    const auto do_part = [&, this](const char *name, Expr part, asmjit::Label &label)
    {
        if (!part)
        {
            return true; // Nothing to compile
        }

        if (const CompileError err = compile_part(comp, part, label); err)
        {
            std::cerr << "Failed to compile part " << name << ":\n"
                      << asmjit::DebugUtils::errorAsString(err.value()) << '\n';
            return false;
        }
        return true;
    };
    if (!do_part("initialize", m_ast->initialize, init_label) //
        || !do_part("iterate", m_ast->iterate, iterate_label) //
        || !do_part("bailout", m_ast->bailout, bailout_label))
    {
        return false;
    }
    if (const CompileError err = emit_data_section(comp, m_state); err)
    {
        std::cerr << "Failed to emit data section:\n" << asmjit::DebugUtils::errorAsString(err.value()) << '\n';
        return false;
    }
    ASMJIT_CHECK(comp.finalize());

    char *module{};
    if (const asmjit::Error err = m_runtime.add(&module, &code); err || !module)
    {
        std::cerr << "Failed to add formula:\n" << asmjit::DebugUtils::errorAsString(err) << '\n';
        return false;
    }
    m_initialize = function_cast<Function *>(code, module, init_label);
    m_iterate = function_cast<Function *>(code, module, iterate_label);
    m_bailout = function_cast<Function *>(code, module, bailout_label);

    return true;
}

Complex ParsedFormula::run(Section part)
{
    auto result = [this](Function *fn)
    {
        if (fn == nullptr)
        {
            return Complex{0.0, 0.0};
        }
        fn();
        return m_state.symbols["_result"];
    };
    switch (part)
    {
    case Section::INITIALIZE:
        return result(m_initialize);
    case Section::ITERATE:
        return result(m_iterate);
    case Section::BAILOUT:
        return result(m_bailout);
    }
    throw std::runtime_error("Invalid part for run");
}

bool valid_sections(const FormulaSectionsPtr &ast)
{
    return ast && (ast->builtin ? !(ast->per_image || ast->initialize || ast->iterate || ast->bailout) : true);
}

} // namespace

FormulaPtr create_formula(std::string_view text)
{
    if (FormulaSectionsPtr sections = parse(text); valid_sections(sections))
    {
        return std::make_shared<ParsedFormula>(sections);
    }
    return {};
}

FormulaPtr create_descent_formula(std::string_view text)
{
    if (FormulaSectionsPtr sections = descent::parse(text); valid_sections(sections))
    {
        return std::make_shared<ParsedFormula>(sections);
    }
    return {};
}

} // namespace formula
