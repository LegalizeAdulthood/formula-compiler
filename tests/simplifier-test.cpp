// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2025 Richard Thomson
//
#include <formula/Simplifier.h>

#include <formula/Node.h>
#include <formula/Visitor.h>

#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

using namespace formula::ast;
using namespace testing;

namespace
{

class NodeFormatter : public Visitor
{
public:
    NodeFormatter(std::ostream &str) :
        m_str(str)
    {
    }
    ~NodeFormatter() override = default;

    void visit(const AssignmentNode &node) override;
    void visit(const BinaryOpNode &node) override;
    void visit(const FunctionCallNode &node) override;
    void visit(const IdentifierNode &node) override;
    void visit(const IfStatementNode &node) override;
    void visit(const NumberNode &node) override;
    void visit(const StatementSeqNode &node) override;
    void visit(const UnaryOpNode &node) override;

private:
    std::ostream &m_str;
};

void NodeFormatter::visit(const AssignmentNode &node)
{
}

void NodeFormatter::visit(const BinaryOpNode &node)
{
}

void NodeFormatter::visit(const FunctionCallNode &node)
{
}

void NodeFormatter::visit(const IdentifierNode &node)
{
    m_str << "identifier:" << node.name() << '\n';
}

void NodeFormatter::visit(const IfStatementNode &node)
{
}

void NodeFormatter::visit(const NumberNode &node)
{
    m_str << "number:" << node.value() << '\n';
}

void NodeFormatter::visit(const StatementSeqNode &node)
{
    for (const Expr &stmt : node.statements())
    {
        stmt->visit(*this);
    }
}

void NodeFormatter::visit(const UnaryOpNode &node)
{
}

class TestFormulaSimplifier : public Test
{
protected:
    std::ostringstream str;
    NodeFormatter formatter{str};
};

std::shared_ptr<NumberNode> number(double value)
{
    return std::make_shared<NumberNode>(value);
}

std::shared_ptr<IdentifierNode> identifier(const std::string &name)
{
    return std::make_shared<IdentifierNode>(name);
}

std::shared_ptr<StatementSeqNode> statements(const std::vector<Expr> &statements)
{
    return std::make_shared<StatementSeqNode>(statements);
}

} // namespace

TEST_F(TestFormulaSimplifier, simplifySingleStatementSequence)
{
    const Expr simplified{formula::simplify(statements({number(42.0)}))};

    simplified->visit(formatter);
    EXPECT_EQ("number:42\n", str.str());
}

TEST_F(TestFormulaSimplifier, multipleStatementSequencePreserved)
{
    const Expr simplified{formula::simplify(statements({number(42.0), identifier("z")}))};

    simplified->visit(formatter);
    EXPECT_EQ("number:42\n"
              "identifier:z\n",
        str.str());
}

TEST_F(TestFormulaSimplifier, collapseMultipleNumberStatements)
{
    const Expr simplified{formula::simplify(statements({identifier("z"), number(42.0), number(7.0), identifier("q")}))};

    simplified->visit(formatter);
    EXPECT_EQ("identifier:z\n"
              "number:7\n"
              "identifier:q\n",
        str.str());
}
