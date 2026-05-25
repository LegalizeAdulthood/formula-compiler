// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025-2026 Richard Thomson
//
#include <formula/translator/GLSLEmitter.h>

#include <formula/core/Visitor.h>

#include <iomanip>
#include <set>
#include <sstream>
#include <unordered_set>

namespace formula::codegen
{

namespace
{

bool is_predefined_variable(const std::string &name)
{
    static const std::unordered_set<std::string> PREDEFINED_VARIABLES{
        "pixel",
        "p1",
        "p2",
        "p3",
        "p4",
        "p5",
        "pi",
        "e",
        "maxit",
        "rand",
        "lastsqr",
        "scrnmax",
        "scrnpix",
        "whitesq",
        "ismand",
        "center",
        "magxmag",
        "rotskew",
    };
    return PREDEFINED_VARIABLES.count(name) > 0;
}

bool is_function_selector(const std::string &name)
{
    return name == "fn1" || name == "fn2" || name == "fn3" || name == "fn4";
}

std::string format_float(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(17) << value;
    std::string text{out.str()};
    while (text.size() > 3 && text.back() == '0' && text[text.size() - 2] != '.')
    {
        text.pop_back();
    }
    if (text == "-0.0")
    {
        return "0.0";
    }
    return text;
}

std::string complex_literal(double real, double imag)
{
    return "vec2(" + format_float(real) + ", " + format_float(imag) + ")";
}

class SymbolCollector : public ast::NullVisitor
{
public:
    void collect(const ast::Expr &expr)
    {
        if (expr)
        {
            expr->visit(*this);
        }
    }

    const std::set<std::string> &symbols() const
    {
        return m_symbols;
    }

    void visit(const ast::AssignmentNode &node) override
    {
        collect_symbol(node.variable());
        collect(node.expression());
    }

    void visit(const ast::BinaryOpNode &node) override
    {
        collect(node.left());
        collect(node.right());
    }

    void visit(const ast::FunctionCallNode &node) override
    {
        if (node.has_target())
        {
            collect(node.target());
        }
        for (const ast::Expr &arg : node.args())
        {
            collect(arg);
        }
    }

    void visit(const ast::IdentifierNode &node) override
    {
        collect_symbol(node.name());
    }

    void visit(const ast::IfStatementNode &node) override
    {
        collect(node.condition());
        if (node.has_then_block())
        {
            collect(node.then_block());
        }
        if (node.has_else_block())
        {
            collect(node.else_block());
        }
    }

    void visit(const ast::StatementSeqNode &node) override
    {
        for (const ast::Expr &statement : node.statements())
        {
            collect(statement);
        }
    }

    void visit(const ast::UnaryOpNode &node) override
    {
        collect(node.operand());
    }

private:
    void collect_symbol(const std::string &name)
    {
        if (name != "z" && !is_predefined_variable(name))
        {
            m_symbols.insert(name);
        }
    }

    std::set<std::string> m_symbols;
};

std::set<std::string> collect_formula_symbols(const ast::FormulaSections &formula)
{
    SymbolCollector collector;
    collector.collect(formula.per_image);
    collector.collect(formula.initialize);
    collector.collect(formula.iterate);
    collector.collect(formula.bailout);
    return collector.symbols();
}

/// This Visitor is an experimental implementation that emits a GLSL compute
/// shader for a BASIC fractal formula.
///
/// The current output is debug/example shader text. It is not a
/// production-correct BASIC backend.
///
/// It merely serves as an example for your own Visitor.
///
class GLSLEmitter : public ast::NullVisitor
{
public:
    GLSLEmitter();
    ~GLSLEmitter() override = default;

    // Generate complete compute shader from formula sections
    std::string emit_shader(const ast::FormulaSections &formula);

    // Visitor methods for each AST node type
    void visit(const ast::AssignmentNode &node) override;
    void visit(const ast::BinaryOpNode &node) override;
    void visit(const ast::FunctionCallNode &node) override;
    void visit(const ast::IdentifierNode &node) override;
    void visit(const ast::IfStatementNode &node) override;
    void visit(const ast::LiteralNode &node) override;
    void visit(const ast::ParamBlockNode &node) override;
    void visit(const ast::SettingNode &node) override;
    void visit(const ast::StatementSeqNode &node) override;
    void visit(const ast::UnaryOpNode &node) override;

    // Get the generated GLSL expression/code
    std::string get_result() const
    {
        return m_output.str();
    }
    void clear_result()
    {
        m_output.str("");
        m_output.clear();
    }

private:
    // Shader generation helpers
    std::string emit_header();
    std::string emit_uniforms();
    std::string emit_complex_math_functions();
    std::string emit_builtin_functions();
    std::string emit_variable_declarations();
    std::string emit_main_function(const ast::FormulaSections &formula);

    // Code emission helpers
    void emit_expression(const ast::Expr &expr);
    void emit_statement(const ast::Expr &stmt);

    // Type system helpers
    bool is_complex_type(const std::string &var_name) const;

    // Builtin function mapping
    std::string map_builtin_function(const std::string &name) const;

    // Output stream for code generation
    std::ostringstream m_output;

    // Variable type tracking
    std::unordered_set<std::string> m_complex_vars;
    std::set<std::string> m_user_vars;

    // Indentation management
    int m_indent_level{0};
    std::string indent() const;

    // Configuration
    int m_workgroup_size_x{8};
    int m_workgroup_size_y{8};
};

GLSLEmitter::GLSLEmitter()
{
    // Mark known complex variables
    m_complex_vars.insert("pixel");
    m_complex_vars.insert("z");
    m_complex_vars.insert("p1");
    m_complex_vars.insert("p2");
    m_complex_vars.insert("p3");
    m_complex_vars.insert("p4");
    m_complex_vars.insert("p5");
}

std::string GLSLEmitter::indent() const
{
    return std::string(m_indent_level * 4, ' ');
}

std::string GLSLEmitter::emit_shader(const ast::FormulaSections &formula)
{
    std::ostringstream shader;
    m_user_vars = collect_formula_symbols(formula);
    m_complex_vars.insert(m_user_vars.begin(), m_user_vars.end());

    // Emit shader components
    shader << emit_header();
    shader << emit_uniforms();
    shader << emit_complex_math_functions();
    shader << emit_builtin_functions();
    shader << emit_main_function(formula);

    return shader.str();
}

std::string GLSLEmitter::emit_header()
{
    std::ostringstream out;

    out << "#version 450\n\n";
    out << "// Auto-generated fractal compute shader\n\n";

    // Workgroup size
    out << "layout(local_size_x = " << m_workgroup_size_x << ", local_size_y = " << m_workgroup_size_y << ") in;\n\n";

    // Output image binding
    out << "layout(rgba32f, binding = 0) uniform image2D output_image;\n\n";

    return out.str();
}

std::string GLSLEmitter::emit_uniforms()
{
    std::ostringstream out;

    out << "// Uniforms\n";
    out << "layout(std140, binding = 1) uniform FractalParams {\n";
    out << "    vec2 p1;          // Parameter 1\n";
    out << "    vec2 p2;          // Parameter 2\n";
    out << "    vec2 p3;          // Parameter 3\n";
    out << "    vec2 p4;          // Parameter 4\n";
    out << "    vec2 p5;          // Parameter 5\n";
    out << "    vec2 center;      // View center\n";
    out << "    vec2 view_size;   // View size\n";
    out << "    uvec2 resolution; // Image resolution\n";
    out << "    uint maxit;       // Max iterations\n";
    out << "    float bailout;    // Bailout radius\n";
    out << "    uint fn1_selector; // Default sin\n";
    out << "    uint fn2_selector; // Default sqr\n";
    out << "    uint fn3_selector; // Default sinh\n";
    out << "    uint fn4_selector; // Default cosh\n";
    out << "};\n\n";

    // Constants
    out << "const float pi = 3.14159265358979323846;\n";
    out << "const float e = 2.71828182845904523536;\n\n";

    out << "const uint FUNCTION_SIN = 0u;\n";
    out << "const uint FUNCTION_COS = 1u;\n";
    out << "const uint FUNCTION_SINH = 2u;\n";
    out << "const uint FUNCTION_COSH = 3u;\n";
    out << "const uint FUNCTION_COSXX = 4u;\n";
    out << "const uint FUNCTION_TAN = 5u;\n";
    out << "const uint FUNCTION_COTAN = 6u;\n";
    out << "const uint FUNCTION_TANH = 7u;\n";
    out << "const uint FUNCTION_COTANH = 8u;\n";
    out << "const uint FUNCTION_SQR = 9u;\n";
    out << "const uint FUNCTION_LOG = 10u;\n";
    out << "const uint FUNCTION_EXP = 11u;\n";
    out << "const uint FUNCTION_ABS = 12u;\n";
    out << "const uint FUNCTION_CONJ = 13u;\n";
    out << "const uint FUNCTION_REAL = 14u;\n";
    out << "const uint FUNCTION_IMAG = 15u;\n";
    out << "const uint FUNCTION_FLIP = 16u;\n";
    out << "const uint FUNCTION_ASIN = 17u;\n";
    out << "const uint FUNCTION_ASINH = 18u;\n";
    out << "const uint FUNCTION_ACOS = 19u;\n";
    out << "const uint FUNCTION_ACOSH = 20u;\n";
    out << "const uint FUNCTION_ATAN = 21u;\n";
    out << "const uint FUNCTION_ATANH = 22u;\n";
    out << "const uint FUNCTION_SQRT = 23u;\n";
    out << "const uint FUNCTION_CABS = 24u;\n";
    out << "const uint FUNCTION_FLOOR = 25u;\n";
    out << "const uint FUNCTION_CEIL = 26u;\n";
    out << "const uint FUNCTION_TRUNC = 27u;\n";
    out << "const uint FUNCTION_ROUND = 28u;\n";
    out << "const uint FUNCTION_IDENT = 29u;\n";
    out << "const uint FUNCTION_ONE = 30u;\n";
    out << "const uint FUNCTION_ZERO = 31u;\n";
    out << "const uint FUNCTION_SRAND = 32u;\n\n";

    return out.str();
}

std::string GLSLEmitter::emit_complex_math_functions()
{
    std::ostringstream out;

    out << "// Complex number operations (vec2 = real + imag * i)\n\n";

    // Basic arithmetic
    out << "vec2 c_add(vec2 a, vec2 b) { return a + b; }\n\n";
    out << "vec2 c_sub(vec2 a, vec2 b) { return a - b; }\n\n";
    out << "vec2 c_neg(vec2 z) { return vec2(-z.x, -z.y); }\n\n";

    out << "vec2 c_mul(vec2 a, vec2 b) {\n";
    out << "    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);\n";
    out << "}\n\n";

    out << "vec2 c_div(vec2 a, vec2 b) {\n";
    out << "    float denom = b.x * b.x + b.y * b.y;\n";
    out << "    return vec2((a.x * b.x + a.y * b.y) / denom,\n";
    out << "                (a.y * b.x - a.x * b.y) / denom);\n";
    out << "}\n\n";

    // Square
    out << "vec2 c_sqr_value(vec2 z) {\n";
    out << "    return vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y);\n";
    out << "}\n\n";

    out << "vec2 c_sqr(vec2 z, inout float lastsqr_value) {\n";
    out << "    lastsqr_value = z.x * z.x + z.y * z.y;\n";
    out << "    return c_sqr_value(z);\n";
    out << "}\n\n";

    // Magnitude operations
    out << "vec2 c_mod_sqr(vec2 z) {\n";
    out << "    return vec2(z.x * z.x + z.y * z.y, 0.0);\n";
    out << "}\n\n";

    out << "vec2 c_mag(vec2 z) {\n";
    out << "    return vec2(sqrt(c_mod_sqr(z).x), 0.0);\n";
    out << "}\n\n";

    out << "vec2 c_abs(vec2 z) {\n";
    out << "    return vec2(abs(z.x), abs(z.y));\n";
    out << "}\n\n";

    out << "vec2 c_cabs(vec2 z) {\n";
    out << "    return c_mag(z);\n";
    out << "}\n\n";

    // Power function
    out << "vec2 c_pow(vec2 base, vec2 exp) {\n";
    out << "    float r = length(base);\n";
    out << "    float theta = atan(base.y, base.x);\n";
    out << "    float log_r = log(r);\n";
    out << "    float a = exp.x * log_r - exp.y * theta;\n";
    out << "    float b = exp.y * log_r + exp.x * theta;\n";
    out << "    float ea = exp(a);\n";
    out << "    return vec2(ea * cos(b), ea * sin(b));\n";
    out << "}\n\n";

    // Exponential and logarithm
    out << "vec2 c_exp(vec2 z) {\n";
    out << "    float e = exp(z.x);\n";
    out << "    return vec2(e * cos(z.y), e * sin(z.y));\n";
    out << "}\n\n";

    out << "vec2 c_log(vec2 z) {\n";
    out << "    return vec2(log(length(z)), atan(z.y, z.x));\n";
    out << "}\n\n";

    // Square root
    out << "vec2 c_sqrt(vec2 z) {\n";
    out << "    float r = length(z);\n";
    out << "    float theta = atan(z.y, z.x);\n";
    out << "    float sr = sqrt(r);\n";
    out << "    return vec2(sr * cos(theta / 2.0), sr * sin(theta / 2.0));\n";
    out << "}\n\n";

    // Trigonometric functions
    out << "vec2 c_sin(vec2 z) {\n";
    out << "    return vec2(sin(z.x) * cosh(z.y), cos(z.x) * sinh(z.y));\n";
    out << "}\n\n";

    out << "vec2 c_cos(vec2 z) {\n";
    out << "    return vec2(cos(z.x) * cosh(z.y), -sin(z.x) * sinh(z.y));\n";
    out << "}\n\n";

    out << "vec2 c_tan(vec2 z) {\n";
    out << "    return c_div(c_sin(z), c_cos(z));\n";
    out << "}\n\n";

    out << "vec2 c_cotan(vec2 z) {\n";
    out << "    return c_div(c_cos(z), c_sin(z));\n";
    out << "}\n\n";

    // Hyperbolic functions
    out << "vec2 c_sinh(vec2 z) {\n";
    out << "    return vec2(sinh(z.x) * cos(z.y), cosh(z.x) * sin(z.y));\n";
    out << "}\n\n";

    out << "vec2 c_cosh(vec2 z) {\n";
    out << "    return vec2(cosh(z.x) * cos(z.y), sinh(z.x) * sin(z.y));\n";
    out << "}\n\n";

    out << "vec2 c_tanh(vec2 z) {\n";
    out << "    return c_div(c_sinh(z), c_cosh(z));\n";
    out << "}\n\n";

    out << "vec2 c_cotanh(vec2 z) {\n";
    out << "    return c_div(c_cosh(z), c_sinh(z));\n";
    out << "}\n\n";

    // Inverse trigonometric functions
    out << "vec2 c_asin(vec2 z) {\n";
    out << "    vec2 i = vec2(0.0, 1.0);\n";
    out << "    vec2 t = c_sqrt(c_sub(vec2(1.0, 0.0), c_sqr_value(z)));\n";
    out << "    return c_mul(vec2(0.0, -1.0), c_log(c_add(c_mul(i, z), t)));\n";
    out << "}\n\n";

    out << "vec2 c_acos(vec2 z) {\n";
    out << "    vec2 i = vec2(0.0, 1.0);\n";
    out << "    vec2 t = c_sqrt(c_sub(vec2(1.0, 0.0), c_sqr_value(z)));\n";
    out << "    return c_mul(vec2(0.0, -1.0), c_log(c_add(z, c_mul(i, t))));\n";
    out << "}\n\n";

    out << "vec2 c_atan(vec2 z) {\n";
    out << "    vec2 i = vec2(0.0, 1.0);\n";
    out << "    return c_mul(vec2(0.0, 0.5), c_log(c_div(c_add(i, z), c_sub(i, z))));\n";
    out << "}\n\n";

    // Inverse hyperbolic functions
    out << "vec2 c_asinh(vec2 z) {\n";
    out << "    return c_log(c_add(z, c_sqrt(c_add(c_sqr_value(z), vec2(1.0, 0.0)))));\n";
    out << "}\n\n";

    out << "vec2 c_acosh(vec2 z) {\n";
    out << "    return c_log(c_add(z, c_sqrt(c_sub(c_sqr_value(z), vec2(1.0, 0.0)))));\n";
    out << "}\n\n";

    out << "vec2 c_atanh(vec2 z) {\n";
    out << "    vec2 one = vec2(1.0, 0.0);\n";
    out << "    return c_mul(vec2(0.5, 0.0), c_log(c_div(c_add(one, z), c_sub(one, z))));\n";
    out << "}\n\n";

    // Component extraction
    out << "vec2 c_real(vec2 z) { return vec2(z.x, 0.0); }\n";
    out << "vec2 c_imag(vec2 z) { return vec2(z.y, 0.0); }\n";
    out << "vec2 c_recip(vec2 z) { return c_div(vec2(1.0, 0.0), z); }\n\n";

    // Conjugate and flip
    out << "vec2 c_conj(vec2 z) {\n";
    out << "    return vec2(z.x, -z.y);\n";
    out << "}\n\n";

    out << "vec2 c_flip(vec2 z) {\n";
    out << "    return vec2(z.y, z.x);\n";
    out << "}\n\n";

    // Special functions
    out << "vec2 c_cosxx(vec2 z) {\n";
    out << "    return vec2(cos(z.x) * cosh(z.y), sin(z.x) * sinh(z.y));\n";
    out << "}\n\n";

    // Identity functions
    out << "vec2 c_ident(vec2 z) { return z; }\n";
    out << "vec2 c_one(vec2 z) { return vec2(1.0, 0.0); }\n";
    out << "vec2 c_zero(vec2 z) { return vec2(0.0, 0.0); }\n\n";

    // Floor, ceil, trunc, round
    out << "vec2 c_floor(vec2 z) { return floor(z); }\n";
    out << "vec2 c_ceil(vec2 z) { return ceil(z); }\n";
    out << "vec2 c_trunc(vec2 z) { return trunc(z); }\n";
    out << "vec2 c_round(vec2 z) { return round(z); }\n\n";

    out << "bool c_truth(vec2 value) { return value.x != 0.0; }\n";
    out << "vec2 c_bool(bool value) { return value ? vec2(1.0, 0.0) : vec2(0.0, 0.0); }\n";
    out << "vec2 c_lt(vec2 a, vec2 b) { return c_bool(a.x < b.x); }\n";
    out << "vec2 c_le(vec2 a, vec2 b) { return c_bool(a.x <= b.x); }\n";
    out << "vec2 c_gt(vec2 a, vec2 b) { return c_bool(a.x > b.x); }\n";
    out << "vec2 c_ge(vec2 a, vec2 b) { return c_bool(a.x >= b.x); }\n";
    out << "vec2 c_eq(vec2 a, vec2 b) { return c_bool(a.x == b.x && a.y == b.y); }\n";
    out << "vec2 c_ne(vec2 a, vec2 b) { return c_bool(a.x != b.x || a.y != b.y); }\n\n";
    out << "vec2 c_and(vec2 a, vec2 b) { return c_bool(a.x != 0.0 && b.x != 0.0); }\n";
    out << "vec2 c_or(vec2 a, vec2 b) { return c_bool(a.x != 0.0 || b.x != 0.0); }\n\n";

    return out.str();
}

std::string GLSLEmitter::emit_builtin_functions()
{
    std::ostringstream out;

    out << "// Additional builtin functions\n";
    out << "// srand random-state reset is not yet implemented\n";
    out << "vec2 c_srand(vec2 z) { return vec2(0.0, 0.0); }\n\n";
    out << "vec2 c_dispatch_function(uint selector, vec2 z, inout float lastsqr_value) {\n";
    out << "    switch (selector) {\n";
    out << "    case FUNCTION_SIN: return c_sin(z);\n";
    out << "    case FUNCTION_COS: return c_cos(z);\n";
    out << "    case FUNCTION_SINH: return c_sinh(z);\n";
    out << "    case FUNCTION_COSH: return c_cosh(z);\n";
    out << "    case FUNCTION_COSXX: return c_cosxx(z);\n";
    out << "    case FUNCTION_TAN: return c_tan(z);\n";
    out << "    case FUNCTION_COTAN: return c_cotan(z);\n";
    out << "    case FUNCTION_TANH: return c_tanh(z);\n";
    out << "    case FUNCTION_COTANH: return c_cotanh(z);\n";
    out << "    case FUNCTION_SQR: return c_sqr(z, lastsqr_value);\n";
    out << "    case FUNCTION_LOG: return c_log(z);\n";
    out << "    case FUNCTION_EXP: return c_exp(z);\n";
    out << "    case FUNCTION_ABS: return c_abs(z);\n";
    out << "    case FUNCTION_CONJ: return c_conj(z);\n";
    out << "    case FUNCTION_REAL: return c_real(z);\n";
    out << "    case FUNCTION_IMAG: return c_imag(z);\n";
    out << "    case FUNCTION_FLIP: return c_flip(z);\n";
    out << "    case FUNCTION_ASIN: return c_asin(z);\n";
    out << "    case FUNCTION_ASINH: return c_asinh(z);\n";
    out << "    case FUNCTION_ACOS: return c_acos(z);\n";
    out << "    case FUNCTION_ACOSH: return c_acosh(z);\n";
    out << "    case FUNCTION_ATAN: return c_atan(z);\n";
    out << "    case FUNCTION_ATANH: return c_atanh(z);\n";
    out << "    case FUNCTION_SQRT: return c_sqrt(z);\n";
    out << "    case FUNCTION_CABS: return c_cabs(z);\n";
    out << "    case FUNCTION_FLOOR: return c_floor(z);\n";
    out << "    case FUNCTION_CEIL: return c_ceil(z);\n";
    out << "    case FUNCTION_TRUNC: return c_trunc(z);\n";
    out << "    case FUNCTION_ROUND: return c_round(z);\n";
    out << "    case FUNCTION_IDENT: return c_ident(z);\n";
    out << "    case FUNCTION_ONE: return c_one(z);\n";
    out << "    case FUNCTION_ZERO: return c_zero(z);\n";
    out << "    case FUNCTION_SRAND: return c_srand(z);\n";
    out << "    }\n";
    out << "    return c_ident(z);\n";
    out << "}\n\n";
    out << "vec2 c_fn1(vec2 z, inout float lastsqr_value) {\n";
    out << "    return c_dispatch_function(fn1_selector, z, lastsqr_value);\n";
    out << "}\n";
    out << "vec2 c_fn2(vec2 z, inout float lastsqr_value) {\n";
    out << "    return c_dispatch_function(fn2_selector, z, lastsqr_value);\n";
    out << "}\n";
    out << "vec2 c_fn3(vec2 z, inout float lastsqr_value) {\n";
    out << "    return c_dispatch_function(fn3_selector, z, lastsqr_value);\n";
    out << "}\n";
    out << "vec2 c_fn4(vec2 z, inout float lastsqr_value) {\n";
    out << "    return c_dispatch_function(fn4_selector, z, lastsqr_value);\n";
    out << "}\n\n";

    return out.str();
}

std::string GLSLEmitter::emit_variable_declarations()
{
    std::ostringstream out;
    for (const std::string &name : m_user_vars)
    {
        out << indent() << "vec2 " << name << " = vec2(0.0, 0.0);\n";
    }
    return out.str();
}

std::string GLSLEmitter::emit_main_function(const ast::FormulaSections &formula)
{
    std::ostringstream out;

    out << "void main() {\n";
    m_indent_level = 1;

    // Get pixel coordinates
    out << indent() << "// Get pixel coordinates\n";
    out << indent() << "ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);\n";
    out << indent() << "if (pixel_coords.x >= resolution.x || pixel_coords.y >= resolution.y)\n";
    out << indent() << "    return;\n\n";

    // Map pixel to complex plane
    out << indent() << "// Map pixel to complex plane\n";
    out << indent() << "vec2 uv = vec2(pixel_coords) / vec2(resolution);\n";
    out << indent() << "vec2 pixel = center + (uv * 2.0 - 1.0) * view_size;\n\n";

    // Per-image initialization (GLOBAL section)
    if (formula.per_image)
    {
        out << indent() << "// Global initialization\n";
        clear_result();
        emit_statement(formula.per_image);
        out << m_output.str();
        out << "\n";
    }

    // Variable initialization (INIT section)
    out << indent() << "// Variable initialization\n";
    out << indent() << "vec2 z = pixel;       // Default initialization\n";
    out << indent() << "float lastsqr_value = 0.0;\n";
    out << indent() << "uint iter = 0u;\n\n";
    if (!m_user_vars.empty())
    {
        out << indent() << "// Formula variables\n";
        out << emit_variable_declarations();
        out << "\n";
    }

    if (formula.initialize)
    {
        clear_result();
        emit_statement(formula.initialize);
        out << m_output.str();
        out << "\n";
    }

    // Main iteration loop
    out << indent() << "// Main iteration loop\n";
    out << indent() << "while (iter < maxit) {\n";
    m_indent_level++;

    if (formula.iterate)
    {
        clear_result();
        emit_statement(formula.iterate);
        out << m_output.str();
        out << "\n";
    }

    // Bailout condition
    out << indent() << "// Bailout test\n";
    if (formula.bailout)
    {
        out << indent() << "if (!c_truth(";
        clear_result();
        emit_expression(formula.bailout);
        out << m_output.str() << ")) break;\n";
    }
    else
    {
        out << indent() << "if (c_mod_sqr(z).x > bailout * bailout) break;\n";
    }

    out << "\n" << indent() << "iter++;\n";
    m_indent_level--;
    out << indent() << "}\n\n";

    // Color output (simple iteration count coloring)
    out << indent() << "// Output color based on iteration count\n";
    out << indent() << "float t = float(iter) / float(maxit);\n";
    out << indent() << "vec4 color = vec4(t, t * t, sqrt(t), 1.0);\n";
    out << indent() << "imageStore(output_image, pixel_coords, color);\n";

    m_indent_level = 0;
    out << "}\n";

    return out.str();
}

void GLSLEmitter::emit_expression(const ast::Expr &expr)
{
    if (expr)
    {
        expr->visit(*this);
    }
}

void GLSLEmitter::emit_statement(const ast::Expr &stmt)
{
    if (stmt)
    {
        stmt->visit(*this);
    }
}

bool GLSLEmitter::is_complex_type(const std::string &var_name) const
{
    return m_complex_vars.count(var_name) > 0;
}

std::string GLSLEmitter::map_builtin_function(const std::string &name) const
{
    // Map formula function names to GLSL complex function names
    return "c_" + name;
}

void GLSLEmitter::visit(const ast::LiteralNode &node)
{
    if (std::holds_alternative<Complex>(node.value()))
    {
        const Complex c{std::get<Complex>(node.value())};
        m_output << complex_literal(c.re, c.im);
    }
    else if (std::holds_alternative<double>(node.value()))
    {
        m_output << complex_literal(std::get<double>(node.value()), 0.0);
    }
    else if (std::holds_alternative<int>(node.value()))
    {
        m_output << complex_literal(static_cast<double>(std::get<int>(node.value())), 0.0);
    }
}

void GLSLEmitter::visit(const ast::IdentifierNode &node)
{
    const std::string &name = node.name();

    if (name == "lastsqr")
    {
        m_output << "vec2(lastsqr_value, 0.0)";
    }
    else
    {
        // Most identifiers pass through unchanged
        m_output << name;
    }
}

void GLSLEmitter::visit(const ast::BinaryOpNode &node)
{
    const std::string &op = node.op();

    // For complex operations, use complex functions
    if (op == "+")
    {
        m_output << "c_add(";
        node.left()->visit(*this);
        m_output << ", ";
        node.right()->visit(*this);
        m_output << ")";
    }
    else if (op == "-")
    {
        m_output << "c_sub(";
        node.left()->visit(*this);
        m_output << ", ";
        node.right()->visit(*this);
        m_output << ")";
    }
    else if (op == "*")
    {
        m_output << "c_mul(";
        node.left()->visit(*this);
        m_output << ", ";
        node.right()->visit(*this);
        m_output << ")";
    }
    else if (op == "/")
    {
        m_output << "c_div(";
        node.left()->visit(*this);
        m_output << ", ";
        node.right()->visit(*this);
        m_output << ")";
    }
    else if (op == "^")
    {
        m_output << "c_pow(";
        node.left()->visit(*this);
        m_output << ", ";
        node.right()->visit(*this);
        m_output << ")";
    }
    else if (op == "<=" || op == "<" || op == ">=" || op == ">" || op == "==" || op == "!=")
    {
        const std::string function = op == "<" ? "c_lt"
            : op == "<="                       ? "c_le"
            : op == ">"                        ? "c_gt"
            : op == ">="                       ? "c_ge"
            : op == "=="                       ? "c_eq"
                                               : "c_ne";
        m_output << function << "(";
        node.left()->visit(*this);
        m_output << ", ";
        node.right()->visit(*this);
        m_output << ")";
    }
    else if (op == "&&" || op == "||")
    {
        m_output << (op == "&&" ? "c_and(" : "c_or(");
        node.left()->visit(*this);
        m_output << ", ";
        node.right()->visit(*this);
        m_output << ")";
    }
}

void GLSLEmitter::visit(const ast::AssignmentNode &node)
{
    m_output << indent() << node.variable() << " = ";
    node.expression()->visit(*this);
    m_output << ";\n";

    // Track if this is a complex variable
    if (is_complex_type(node.variable()))
    {
        // Already tracked
    }
    else
    {
        // Assume complex for now - in a full implementation, we'd do type inference
        m_complex_vars.insert(node.variable());
    }
}

void GLSLEmitter::visit(const ast::FunctionCallNode &node)
{
    std::string glsl_func = map_builtin_function(node.name());
    m_output << glsl_func << "(";
    node.arg()->visit(*this);
    if (node.name() == "sqr" || is_function_selector(node.name()))
    {
        m_output << ", lastsqr_value";
    }
    m_output << ")";
}

void GLSLEmitter::visit(const ast::UnaryOpNode &node)
{
    if (node.op() == '-')
    {
        m_output << "c_neg(";
        node.operand()->visit(*this);
        m_output << ")";
    }
    else if (node.op() == '+')
    {
        node.operand()->visit(*this);
    }
    else if (node.op() == '|')
    {
        m_output << "c_mod_sqr(";
        node.operand()->visit(*this);
        m_output << ")";
    }
}

void GLSLEmitter::visit(const ast::IfStatementNode &node)
{
    m_output << indent() << "if (c_truth(";
    node.condition()->visit(*this);
    m_output << ")) {\n";

    if (node.has_then_block())
    {
        m_indent_level++;
        node.then_block()->visit(*this);
        m_indent_level--;
    }

    if (node.has_else_block())
    {
        m_output << indent() << "} else {\n";
        m_indent_level++;
        node.else_block()->visit(*this);
        m_indent_level--;
    }

    m_output << indent() << "}\n";
}

void GLSLEmitter::visit(const ast::StatementSeqNode &node)
{
    for (const auto &stmt : node.statements())
    {
        emit_statement(stmt);
    }
}

void GLSLEmitter::visit(const ast::SettingNode &)
{
    // Settings are not emitted in shader code
}

void GLSLEmitter::visit(const ast::ParamBlockNode &)
{
    // Parameter blocks are not emitted in shader code
}

} // namespace

std::string emit_shader(const ast::FormulaSections &formula)
{
    GLSLEmitter emitter;
    return emitter.emit_shader(formula);
}

} // namespace formula::codegen
