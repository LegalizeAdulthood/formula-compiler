#pragma once

#include <formula/Complex.h>

#include <asmjit/core.h>
#include <asmjit/x86.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace formula::ast
{

using SymbolTable = std::map<std::string, Complex>;

struct LabelBinding
{
    asmjit::Label label;
    bool bound;
};

using ConstantBindings = std::map<double, LabelBinding>;
using SymbolBindings = std::map<std::string, LabelBinding>;

struct DataSection
{
    asmjit::Section *data{};    // Section for data storage
    ConstantBindings constants; // Map of constants to labels
    SymbolBindings symbols;     // Map of symbols to labels
};

struct EmitterState
{
    SymbolTable symbols;
    DataSection data;
};

class Node
{
public:
    virtual ~Node() = default;

    virtual Complex interpret(SymbolTable &symbols) const = 0;
    virtual bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const = 0;
};

using Expr = std::shared_ptr<Node>;

class NumberNode : public Node
{
public:
    NumberNode(double value) :
        m_value(value)
    {
    }
    ~NumberNode() override = default;

    Complex interpret(SymbolTable &) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    double m_value{};
};

class IdentifierNode : public Node
{
public:
    IdentifierNode(std::string value) :
        m_name(std::move(value))
    {
    }
    ~IdentifierNode() override = default;

    Complex interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    std::string m_name;
};

class FunctionCallNode : public Node
{
public:
    FunctionCallNode(std::string name, Expr arg) :
        m_name(std::move(name)),
        m_arg(std::move(arg))
    {
    }
    ~FunctionCallNode() override = default;

    Complex interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    std::string m_name;
    Expr m_arg;
};

class UnaryOpNode : public Node
{
public:
    UnaryOpNode(char op, Expr operand) :
        m_op(op),
        m_operand(std::move(operand))
    {
    }
    ~UnaryOpNode() override = default;

    Complex interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    char m_op;
    Expr m_operand;
};

class BinaryOpNode : public Node
{
public:
    BinaryOpNode() = default;
    BinaryOpNode(Expr left, char op, Expr right) :
        m_left(std::move(left)),
        m_op(1, op),
        m_right(std::move(right))
    {
    }
    BinaryOpNode(Expr left, std::string op, Expr right) :
        m_left(std::move(left)),
        m_op(std::move(op)),
        m_right(std::move(right))
    {
    }
    ~BinaryOpNode() override = default;

    Complex interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    Expr m_left;
    std::string m_op;
    Expr m_right;
};

class AssignmentNode : public Node
{
public:
    AssignmentNode(std::string variable, Expr expression) :
        m_variable(std::move(variable)),
        m_expression(std::move(expression))
    {
    }
    ~AssignmentNode() override = default;

    Complex interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    std::string m_variable;
    Expr m_expression;
};

class StatementSeqNode : public Node
{
public:
    StatementSeqNode(std::vector<Expr> statements) :
        m_statements(std::move(statements))
    {
    }
    ~StatementSeqNode() override = default;

    Complex interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    std::vector<Expr> m_statements;
};

class IfStatementNode : public Node
{
public:
    IfStatementNode(Expr condition, Expr then_block, Expr else_block) :
        m_condition(condition),
        m_then_block(then_block),
        m_else_block(else_block)
    {
    }
    ~IfStatementNode() override = default;

    Complex interpret(SymbolTable &symbols) const override;
    bool compile(asmjit::x86::Compiler &comp, EmitterState &state, asmjit::x86::Xmm result) const override;

private:
    Expr m_condition;
    Expr m_then_block;
    Expr m_else_block;
};

struct FormulaDefinition
{
    Expr initialize;
    Expr iterate;
    Expr bailout;
};

} // namespace formula
