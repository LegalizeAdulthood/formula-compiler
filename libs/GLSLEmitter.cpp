// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/GLSLEmitter.h>

#include <formula/Visitor.h>

#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace formula::codegen
{

namespace
{

/// This Visitor is an example implementation that emits a GLSL compute
/// shader for the fractal formula.
///
/// This example was generated with Copilot and not much else was done,
/// so the results may be incorrect and buggy as an actual GLSL compute
/// shader.
///
/// It merely serves as an example for your own Visitor.
///
class GLSLEmitter : public ast::Visitor
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
    m_complex_vars.insert("c");
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
    out << "};\n\n";

    // Constants
    out << "const float pi = 3.14159265358979323846;\n";
    out << "const float e = 2.71828182845904523536;\n\n";

    return out.str();
}

std::string GLSLEmitter::emit_complex_math_functions()
{
    std::ostringstream out;

    out << "// Complex number operations (vec2 = real + imag * i)\n\n";

    // Basic arithmetic
    out << "vec2 c_add(vec2 a, vec2 b) { return a + b; }\n\n";
    out << "vec2 c_sub(vec2 a, vec2 b) { return a - b; }\n\n";

    out << "vec2 c_mul(vec2 a, vec2 b) {\n";
    out << "    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);\n";
    out << "}\n\n";

    out << "vec2 c_div(vec2 a, vec2 b) {\n";
    out << "    float denom = b.x * b.x + b.y * b.y;\n";
    out << "    return vec2((a.x * b.x + a.y * b.y) / denom,\n";
    out << "                (a.y * b.x - a.x * b.y) / denom);\n";
    out << "}\n\n";

    // Square
    out << "vec2 c_sqr(vec2 z) {\n";
    out << "    return vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y);\n";
    out << "}\n\n";

    // Magnitude operations
    out << "float c_mag_sqr(vec2 z) {\n";
    out << "    return z.x * z.x + z.y * z.y;\n";
    out << "}\n\n";

    out << "float c_abs(vec2 z) {\n";
    out << "    return sqrt(c_mag_sqr(z));\n";
    out << "}\n\n";

    out << "float c_cabs(vec2 z) {\n";
    out << "    return c_abs(z);\n";
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
    out << "    vec2 t = c_sqrt(c_sub(vec2(1.0, 0.0), c_sqr(z)));\n";
    out << "    return c_mul(vec2(0.0, -1.0), c_log(c_add(c_mul(i, z), t)));\n";
    out << "}\n\n";

    out << "vec2 c_acos(vec2 z) {\n";
    out << "    vec2 i = vec2(0.0, 1.0);\n";
    out << "    vec2 t = c_sqrt(c_sub(vec2(1.0, 0.0), c_sqr(z)));\n";
    out << "    return c_mul(vec2(0.0, -1.0), c_log(c_add(z, c_mul(i, t))));\n";
    out << "}\n\n";

    out << "vec2 c_atan(vec2 z) {\n";
    out << "    vec2 i = vec2(0.0, 1.0);\n";
    out << "    return c_mul(vec2(0.0, 0.5), c_log(c_div(c_add(i, z), c_sub(i, z))));\n";
    out << "}\n\n";

    // Inverse hyperbolic functions
    out << "vec2 c_asinh(vec2 z) {\n";
    out << "    return c_log(c_add(z, c_sqrt(c_add(c_sqr(z), vec2(1.0, 0.0)))));\n";
    out << "}\n\n";

    out << "vec2 c_acosh(vec2 z) {\n";
    out << "    return c_log(c_add(z, c_sqrt(c_sub(c_sqr(z), vec2(1.0, 0.0)))));\n";
    out << "}\n\n";

    out << "vec2 c_atanh(vec2 z) {\n";
    out << "    vec2 one = vec2(1.0, 0.0);\n";
    out << "    return c_mul(vec2(0.5, 0.0), c_log(c_div(c_add(one, z), c_sub(one, z))));\n";
    out << "}\n\n";

    // Component extraction
    out << "float c_real(vec2 z) { return z.x; }\n\n";
    out << "float c_imag(vec2 z) { return z.y; }\n\n";

    // Conjugate and flip
    out << "vec2 c_conj(vec2 z) {\n";
    out << "    return vec2(z.x, -z.y);\n";
    out << "}\n\n";

    out << "vec2 c_flip(vec2 z) {\n";
    out << "    return vec2(-z.y, z.x);\n";
    out << "}\n\n";

    // Special functions
    out << "vec2 c_cosxx(vec2 z) {\n";
    out << "    // cosxx is cos(x)*cosh(y) (real part of complex cosine)\n";
    out << "    return vec2(cos(z.x) * cosh(z.y), 0.0);\n";
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

    return out.str();
}

std::string GLSLEmitter::emit_builtin_functions()
{
    std::ostringstream out;

    out << "// Additional builtin functions\n";
    out << "// fn1, fn2, fn3, fn4 are user-configurable via uniforms (not yet implemented)\n";
    out << "vec2 c_fn1(vec2 z) { return c_ident(z); }\n";
    out << "vec2 c_fn2(vec2 z) { return c_ident(z); }\n";
    out << "vec2 c_fn3(vec2 z) { return c_ident(z); }\n";
    out << "vec2 c_fn4(vec2 z) { return c_ident(z); }\n\n";

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
    out << indent() << "float lastsqr = 0.0;\n";
    out << indent() << "uint iter = 0u;\n\n";

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
        out << indent() << "if (!(";
        clear_result();
        emit_expression(formula.bailout);
        out << m_output.str() << ")) break;\n";
    }
    else
    {
        out << indent() << "if (c_mag_sqr(z) > bailout * bailout) break;\n";
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
        auto c = std::get<Complex>(node.value());
        m_output << "vec2(" << std::fixed << std::setprecision(17) << c.re << ", " << c.im << ")";
    }
    else if (std::holds_alternative<double>(node.value()))
    {
        double val = std::get<double>(node.value());
        m_output << std::fixed << std::setprecision(17) << val;
    }
    else if (std::holds_alternative<int>(node.value()))
    {
        m_output << std::get<int>(node.value());
    }
}

void GLSLEmitter::visit(const ast::IdentifierNode &node)
{
    const std::string &name = node.name();

    // Special case for lastsqr - use underscore for GLSL convention
    if (name == "lastsqr")
    {
        m_output << "lastsqr";
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
        // Comparison operations (scalar)
        m_output << "(";
        node.left()->visit(*this);
        m_output << " " << op << " ";
        node.right()->visit(*this);
        m_output << ")";
    }
    else if (op == "&&" || op == "||")
    {
        // Logical operations
        m_output << "(";
        node.left()->visit(*this);
        m_output << " " << op << " ";
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
    m_output << ")";
}

void GLSLEmitter::visit(const ast::UnaryOpNode &node)
{
    if (node.op() == '-')
    {
        m_output << "(-";
        node.operand()->visit(*this);
        m_output << ")";
    }
    else if (node.op() == '+')
    {
        m_output << "(+";
        node.operand()->visit(*this);
        m_output << ")";
    }
    else if (node.op() == '|')
    {
        // Modulus/absolute value
        m_output << "c_abs(";
        node.operand()->visit(*this);
        m_output << ")";
    }
}

void GLSLEmitter::visit(const ast::IfStatementNode &node)
{
    m_output << indent() << "if (";
    node.condition()->visit(*this);
    m_output << ") {\n";

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
