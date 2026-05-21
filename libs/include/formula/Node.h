// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#pragma once

#include <formula/Complex.h>
#include <formula/SourceLocation.h>

#include <cassert>
#include <memory>
#include <optional>
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

class LiteralNode : public Node
{
public:
    struct Color
    {
        double red{};
        double green{};
        double blue{};
        double alpha{1.0};
    };

    using ValueType = std::variant<int, double, Complex, bool, std::string, Color>;
    explicit LiteralNode(int value) :
        m_value(value)
    {
    }
    explicit LiteralNode(double value) :
        m_value(value)
    {
    }
    explicit LiteralNode(Complex value) :
        m_value(value)
    {
    }
    explicit LiteralNode(bool value) :
        m_value(value)
    {
    }
    explicit LiteralNode(std::string value) :
        m_value(std::move(value))
    {
    }
    explicit LiteralNode(Color value) :
        m_value(value)
    {
    }
    ~LiteralNode() override = default;

    void visit(Visitor &visitor) const override;

    ValueType value() const
    {
        return m_value;
    }

private:
    ValueType m_value{};
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
        m_args{std::move(arg)}
    {
    }
    FunctionCallNode(std::string name, std::vector<Expr> args) :
        m_name(std::move(name)),
        m_args(std::move(args))
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
        assert(!m_args.empty());
        return m_args.front();
    }
    const std::vector<Expr> &args() const
    {
        return m_args;
    }

private:
    std::string m_name;
    std::vector<Expr> m_args;
};

class ParameterRefNode : public Node
{
public:
    explicit ParameterRefNode(std::string name) :
        m_name(std::move(name))
    {
    }
    ~ParameterRefNode() override = default;

    void visit(Visitor &visitor) const override;

    const std::string &name() const
    {
        return m_name;
    }

private:
    std::string m_name;
};

class ConstantRefNode : public Node
{
public:
    explicit ConstantRefNode(std::string name) :
        m_name(std::move(name))
    {
    }
    ~ConstantRefNode() override = default;

    void visit(Visitor &visitor) const override;

    const std::string &name() const
    {
        return m_name;
    }

private:
    std::string m_name;
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
        m_target(std::make_shared<IdentifierNode>(variable)),
        m_variable(std::move(variable)),
        m_expression(std::move(expression))
    {
    }
    AssignmentNode(Expr target, Expr expression) :
        m_target(std::move(target)),
        m_expression(std::move(expression))
    {
        if (const auto *identifier = dynamic_cast<const IdentifierNode *>(m_target.get()); identifier)
        {
            m_variable = identifier->name();
        }
    }
    ~AssignmentNode() override = default;

    void visit(Visitor &visitor) const override;

    const std::string &variable() const
    {
        return m_variable;
    }
    const Expr &target() const
    {
        return m_target;
    }
    const Expr &expression() const
    {
        return m_expression;
    }

private:
    Expr m_target;
    std::string m_variable;
    Expr m_expression;
};

class IndexNode : public Node
{
public:
    IndexNode(Expr target, std::vector<Expr> indices) :
        m_target(std::move(target)),
        m_indices(std::move(indices))
    {
    }
    ~IndexNode() override = default;

    void visit(Visitor &visitor) const override;

    const Expr &target() const
    {
        return m_target;
    }
    const std::vector<Expr> &indices() const
    {
        return m_indices;
    }

private:
    Expr m_target;
    std::vector<Expr> m_indices;
};

class MemberAccessNode : public Node
{
public:
    MemberAccessNode(Expr target, std::string member) :
        m_target(std::move(target)),
        m_member(std::move(member))
    {
    }
    ~MemberAccessNode() override = default;

    void visit(Visitor &visitor) const override;

    const Expr &target() const
    {
        return m_target;
    }
    const std::string &member() const
    {
        return m_member;
    }

private:
    Expr m_target;
    std::string m_member;
};

class NewNode : public Node
{
public:
    NewNode(std::string type, std::vector<Expr> args) :
        m_type(std::move(type)),
        m_args(std::move(args))
    {
    }
    ~NewNode() override = default;

    void visit(Visitor &visitor) const override;

    const std::string &type() const
    {
        return m_type;
    }
    const std::vector<Expr> &args() const
    {
        return m_args;
    }

private:
    std::string m_type;
    std::vector<Expr> m_args;
};

class DeclarationNode : public Node
{
public:
    DeclarationNode(std::string type, std::string name, std::vector<Expr> dimensions, Expr initializer) :
        m_type(std::move(type)),
        m_name(std::move(name)),
        m_dimensions(std::move(dimensions)),
        m_initializer(std::move(initializer))
    {
    }
    ~DeclarationNode() override = default;

    void visit(Visitor &visitor) const override;

    const std::string &type() const
    {
        return m_type;
    }
    const std::string &name() const
    {
        return m_name;
    }
    const std::vector<Expr> &dimensions() const
    {
        return m_dimensions;
    }
    bool is_array() const
    {
        return !m_dimensions.empty();
    }
    bool is_dynamic_array() const
    {
        return is_array() && !m_dimensions.front();
    }
    const Expr &initializer() const
    {
        return m_initializer;
    }

private:
    std::string m_type;
    std::string m_name;
    std::vector<Expr> m_dimensions;
    Expr m_initializer;
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

class WhileNode : public Node
{
public:
    WhileNode(Expr condition, Expr body) :
        m_condition(std::move(condition)),
        m_body(std::move(body))
    {
    }
    ~WhileNode() override = default;

    void visit(Visitor &visitor) const override;

    const Expr &condition() const
    {
        return m_condition;
    }
    const Expr &body() const
    {
        return m_body;
    }

private:
    Expr m_condition;
    Expr m_body;
};

class RepeatUntilNode : public Node
{
public:
    RepeatUntilNode(Expr body, Expr condition) :
        m_body(std::move(body)),
        m_condition(std::move(condition))
    {
    }
    ~RepeatUntilNode() override = default;

    void visit(Visitor &visitor) const override;

    const Expr &body() const
    {
        return m_body;
    }
    const Expr &condition() const
    {
        return m_condition;
    }

private:
    Expr m_body;
    Expr m_condition;
};

class ReturnNode : public Node
{
public:
    explicit ReturnNode(Expr expression) :
        m_expression(std::move(expression))
    {
    }
    ~ReturnNode() override = default;

    void visit(Visitor &visitor) const override;

    const Expr &expression() const
    {
        return m_expression;
    }

private:
    Expr m_expression;
};

struct FunctionArgument
{
    bool is_const{};
    bool is_by_ref{};
    std::string type;
    std::string name;
};

struct ImportDirective
{
    std::string filename;
    SourceLocation location;
    bool implicit{};
};

class FunctionDeclNode : public Node
{
public:
    FunctionDeclNode(std::string return_type, std::string name, std::vector<FunctionArgument> args, Expr body,
        bool is_const = false, bool is_static = false) :
        m_return_type(std::move(return_type)),
        m_name(std::move(name)),
        m_args(std::move(args)),
        m_body(std::move(body)),
        m_is_const(is_const),
        m_is_static(is_static)
    {
    }
    ~FunctionDeclNode() override = default;

    void visit(Visitor &visitor) const override;

    const std::string &return_type() const
    {
        return m_return_type;
    }
    const std::string &name() const
    {
        return m_name;
    }
    const std::vector<FunctionArgument> &args() const
    {
        return m_args;
    }
    const Expr &body() const
    {
        return m_body;
    }
    bool is_const() const
    {
        return m_is_const;
    }
    bool is_static() const
    {
        return m_is_static;
    }

private:
    std::string m_return_type;
    std::string m_name;
    std::vector<FunctionArgument> m_args;
    Expr m_body;
    bool m_is_const{};
    bool m_is_static{};
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

struct EnumName
{
    std::string name;
};

struct SwitchParam
{
    std::string src;
};

class SettingNode : public Node
{
public:
    using ValueType = std::variant<double, Complex, std::string, int, //
        EnumName, bool, Expr, std::vector<std::string>, SwitchParam>;

    SettingNode(std::string key, int value) :
        m_key(std::move(key)),
        m_value(value)
    {
    }
    SettingNode(std::string key, double value) :
        m_key(std::move(key)),
        m_value(value)
    {
    }
    SettingNode(std::string key, Complex value) :
        m_key(std::move(key)),
        m_value(value)
    {
    }
    SettingNode(std::string key, std::string_view value) :
        m_key(std::move(key)),
        m_value(std::string{value})
    {
    }
    SettingNode(std::string key, EnumName value) :
        m_key(std::move(key)),
        m_value(std::move(value))
    {
    }
    SettingNode(std::string key, bool value) :
        m_key(std::move(key)),
        m_value(value)
    {
    }
    SettingNode(std::string key, Expr value) :
        m_key(std::move(key)),
        m_value(std::move(value))
    {
    }
    SettingNode(std::string key, std::vector<std::string> value) :
        m_key(std::move(key)),
        m_value(std::move(value))
    {
    }
    SettingNode(std::string key, SwitchParam value) :
        m_key(std::move(key)),
        m_value(std::move(value))
    {
    }
    ~SettingNode() override = default;
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

class ParamBlockNode : public Node
{
public:
    ParamBlockNode(std::string type, std::string name, const Expr &block) :
        m_type(std::move(type)),
        m_name(std::move(name)),
        m_block(block)
    {
    }
    ~ParamBlockNode() override = default;
    void visit(Visitor &visitor) const override;

    const std::string &type() const
    {
        return m_type;
    }
    const std::string &name() const
    {
        return m_name;
    }
    const Expr &block() const
    {
        return m_block;
    }

private:
    std::string m_type;
    std::string m_name;
    Expr m_block;
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
    Expr final;
    Expr transform;
    Expr public_members;
    Expr protected_members;
    Expr private_members;
    std::vector<ImportDirective> imports;
};

using FormulaSectionsPtr = std::shared_ptr<FormulaSections>;

} // namespace formula::ast
