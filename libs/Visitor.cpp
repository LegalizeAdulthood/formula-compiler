// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include "Visitor.h"

#include "ast.h"

#include <map>
#include <string>

namespace formula::ast
{

using Dictionary = std::map<std::string, Complex>;

namespace
{

class Interpreter : public Visitor
{
public:
    Interpreter() = default;
    ~Interpreter() override = default;

    void visit(const FormulaDefinition &definition) override;
    void visit(const AssignmentNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const FunctionCallNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const IfStatementNode &node) override;
    void visit(const NumberNode &node) override;
    void visit(const StatementSeqNode &node) override;
    void visit(const UnaryOpNode &node) override;

    Complex result() const
    {
        return m_result;
    }

private:
    Complex m_result;
};

} // namespace

Complex interpret(const std::shared_ptr<Node> &expr)
{
    Interpreter interp;
    expr->visit(interp);
    return interp.result();
}

} // namespace formula::ast
