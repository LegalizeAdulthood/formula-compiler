// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Complex.h>

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace formula::ast
{

class Visitor;

class Node
{
public:
    virtual ~Node() = default;

    virtual void visit(Visitor &visitor) const = 0;
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

    void visit(Visitor &visitor) const override;

    double value() const
    {
        return m_value;
    }

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

    void visit(Visitor &visitor) const override;

    const std::string &name() const
    {
        return m_name;
    }

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

    void visit(Visitor &visitor) const override;

    const std::string &name() const
    {
        return m_name;
    }
    const Expr &arg() const
    {
        return m_arg;
    }

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

    void visit(Visitor &visitor) const override;

    char op() const
    {
        return m_op;
    }
    const Expr &operand() const
    {
        return m_operand;
    }

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

    void visit(Visitor &visitor) const override;

    const Expr &left() const
    {
        return m_left;
    }
    const std::string &op() const
    {
        return m_op;
    }
    const Expr &right() const
    {
        return m_right;
    }

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

    void visit(Visitor &visitor) const override;

    const std::string &variable() const
    {
        return m_variable;
    }
    const Expr &expression() const
    {
        return m_expression;
    }

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

    void visit(Visitor &visitor) const override;

    const std::vector<Expr> &statements() const
    {
        return m_statements;
    }

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

    void visit(Visitor &visitor) const override;

    const Expr &condition() const
    {
        return m_condition;
    }
    bool has_then_block() const
    {
        return static_cast<bool>(m_then_block);
    }
    const Expr &then_block() const
    {
        assert(m_then_block);
        return m_then_block;
    }
    bool has_else_block() const
    {
        return static_cast<bool>(m_else_block);
    }
    const Expr &else_block() const
    {
        assert(m_else_block);
        return m_else_block;
    }

private:
    Expr m_condition;
    Expr m_then_block;
    Expr m_else_block;
};

enum class BuiltinType
{
    MANDELBROT = 1,
    JULIA = 2
};

inline int operator+(BuiltinType value)
{
    return static_cast<int>(value);
}

class TypeNode : public Node
{
public:
    explicit TypeNode(BuiltinType type) :
        m_type(type)
    {
    }
    ~TypeNode() override = default;
    void visit(Visitor &visitor) const override;

    BuiltinType type() const
    {
        return m_type;
    }

private:
    BuiltinType m_type;
};

class DefaultNode : public Node
{
public:
    using ValueType = std::variant<int, Complex, std::string>;

    DefaultNode(std::string key, int value) :
        m_key(std::move(key)),
        m_value(value)
    {
    }
    DefaultNode(std::string key, Complex value) :
        m_key(std::move(key)),
        m_value(value)
    {
    }
    DefaultNode(std::string key, std::string_view value) :
        m_key(std::move(key)),
        m_value(std::string{value})
    {
    }
    ~DefaultNode() override = default;
    void visit(Visitor &visitor) const override;

    const std::string &key() const
    {
        return m_key;
    }
    const ValueType &value() const
    {
        return m_value;
    }

private:
    std::string m_key;
    ValueType m_value;
};

struct FormulaSections
{
    Expr per_image;
    Expr builtin;
    Expr initialize;
    Expr iterate;
    Expr bailout;
    Expr perturb_initialize;
    Expr perturb_iterate;
    Expr defaults;
    Expr type_switch;
};

} // namespace formula::ast
