// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include <formula/ReferenceCollector.h>

#include <formula/ParseOptions.h>
#include <formula/Parser.h>
#include <formula/Visitor.h>

#include <algorithm>
#include <cctype>
#include <string_view>

namespace formula
{

namespace
{

bool is_builtin_type(std::string_view type)
{
    std::string lower;
    lower.reserve(type.size());
    std::transform(type.begin(), type.end(), std::back_inserter(lower),
        [](char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); });

    static constexpr std::string_view builtins[]{
        "bool",
        "color",
        "complex",
        "float",
        "gradient",
        "image",
        "int",
        "string",
        "void",
    };

    return std::find(std::begin(builtins), std::end(builtins), lower) != std::end(builtins);
}

bool is_class_reference(std::string_view type)
{
    return !type.empty() && !is_builtin_type(type);
}

class ReferenceCollector : public ast::NullVisitor
{
public:
    const std::vector<FormulaReference> &references() const
    {
        return m_references;
    }

    void visit(const ast::AssignmentNode &node) override
    {
        visit_expr(node.target());
        visit_expr(node.expression());
    }

    void visit(const ast::BinaryOpNode &node) override
    {
        visit_expr(node.left());
        visit_expr(node.right());
    }

    void visit(const ast::DeclarationNode &node) override
    {
        add_reference(FormulaReferenceKind::DECLARATION, node.type());
        for (const ast::Expr &dimension : node.dimensions())
        {
            visit_expr(dimension);
        }
        visit_expr(node.initializer());
    }

    void visit(const ast::FunctionCallNode &node) override
    {
        for (const ast::Expr &arg : node.args())
        {
            visit_expr(arg);
        }
    }

    void visit(const ast::FunctionDeclNode &node) override
    {
        add_reference(FormulaReferenceKind::FUNCTION_RETURN, node.return_type());
        for (const ast::FunctionArgument &arg : node.args())
        {
            add_reference(FormulaReferenceKind::FUNCTION_ARGUMENT, arg.type);
        }
        visit_expr(node.body());
    }

    void visit(const ast::IfStatementNode &node) override
    {
        visit_expr(node.condition());
        if (node.has_then_block())
        {
            visit_expr(node.then_block());
        }
        if (node.has_else_block())
        {
            visit_expr(node.else_block());
        }
    }

    void visit(const ast::IndexNode &node) override
    {
        visit_expr(node.target());
        for (const ast::Expr &index : node.indices())
        {
            visit_expr(index);
        }
    }

    void visit(const ast::MemberAccessNode &node) override
    {
        visit_expr(node.target());
    }

    void visit(const ast::NewNode &node) override
    {
        add_reference(FormulaReferenceKind::NEW_OBJECT, node.type());
        for (const ast::Expr &arg : node.args())
        {
            visit_expr(arg);
        }
    }

    void visit(const ast::ParamBlockNode &node) override
    {
        visit_expr(node.block());
    }

    void visit(const ast::RepeatUntilNode &node) override
    {
        visit_expr(node.body());
        visit_expr(node.condition());
    }

    void visit(const ast::ReturnNode &node) override
    {
        visit_expr(node.expression());
    }

    void visit(const ast::SettingNode &node) override
    {
        if (const auto *expr = std::get_if<ast::Expr>(&node.value()))
        {
            visit_expr(*expr);
        }
    }

    void visit(const ast::StatementSeqNode &node) override
    {
        for (const ast::Expr &statement : node.statements())
        {
            visit_expr(statement);
        }
    }

    void visit(const ast::UnaryOpNode &node) override
    {
        visit_expr(node.operand());
    }

    void visit(const ast::WhileNode &node) override
    {
        visit_expr(node.condition());
        visit_expr(node.body());
    }

private:
    void add_reference(FormulaReferenceKind kind, std::string_view type)
    {
        if (is_class_reference(type))
        {
            m_references.push_back(FormulaReference{kind, std::string{type}, {}});
        }
    }

    void visit_expr(const ast::Expr &expr)
    {
        if (expr)
        {
            expr->visit(*this);
        }
    }

    std::vector<FormulaReference> m_references;
};

void collect_section_references(ReferenceCollector &collector, const ast::FormulaSections &ast)
{
    const ast::Expr sections[]{
        ast.per_image,
        ast.builtin,
        ast.initialize,
        ast.iterate,
        ast.bailout,
        ast.perturb_initialize,
        ast.perturb_iterate,
        ast.defaults,
        ast.type_switch,
        ast.final,
        ast.transform,
        ast.public_members,
        ast.protected_members,
        ast.private_members,
    };

    for (const ast::Expr &section : sections)
    {
        if (section)
        {
            section->visit(collector);
        }
    }
}

void add_parse_errors(FormulaFileSet &files, std::string_view filename, const parser::ParserPtr &parser)
{
    for (const parser::Diagnostic &error : parser->get_errors())
    {
        SourceLocation location{error.position};
        if (location.filename.empty())
        {
            location.filename = std::string{filename};
        }
        files.diagnostics.push_back(FormulaFileDiagnostic{FormulaFileDiagnosticCode::PARSE_ERROR, std::string{filename},
            std::move(location), parser::to_string(error.code), {}});
    }
}

const ClassHeader *find_class_header(const FormulaFile &file, std::size_t entry_index)
{
    for (const ClassHeader &klass : file.classes)
    {
        if (klass.entry_index == entry_index)
        {
            return &klass;
        }
    }
    return nullptr;
}

} // namespace

std::vector<FormulaReference> collect_formula_references(const ast::FormulaSections &ast)
{
    ReferenceCollector collector;
    collect_section_references(collector, ast);
    return collector.references();
}

std::vector<FormulaReference> collect_formula_references(const ClassHeader &header, const ast::FormulaSections &ast)
{
    std::vector<FormulaReference> result;
    if (is_class_reference(header.base_class))
    {
        result.push_back(FormulaReference{FormulaReferenceKind::BASE_CLASS, header.base_class, {}});
    }

    std::vector<FormulaReference> ast_references{collect_formula_references(ast)};
    result.insert(result.end(), ast_references.begin(), ast_references.end());
    return result;
}

FormulaEntryReferences collect_formula_entry_references(
    FormulaFileSet &files, std::size_t file_index, std::size_t entry_index)
{
    FormulaEntryReferences result;
    result.file_index = file_index;
    result.entry_index = entry_index;
    if (file_index >= files.files.size())
    {
        files.diagnostics.push_back(FormulaFileDiagnostic{FormulaFileDiagnosticCode::PARSE_ERROR, {}, {}, {}, {}});
        return result;
    }

    const FormulaFile &file = files.files[file_index];
    result.filename = file.filename;
    if (entry_index >= file.entries.size())
    {
        files.diagnostics.push_back(
            FormulaFileDiagnostic{FormulaFileDiagnosticCode::PARSE_ERROR, file.filename, {}, {}, {}});
        return result;
    }

    const FormulaEntry &entry = file.entries[entry_index];
    parser::Options options;
    options.source_filename = file.filename;
    options.entry_kind = entry.is_class ? parser::EntryKind::CLASS : parser::EntryKind::FRACTAL;

    const parser::ParserPtr parser{parser::create_parser(entry.body, options)};
    const ast::FormulaSectionsPtr ast{parser->parse()};
    if (!parser->get_errors().empty())
    {
        add_parse_errors(files, file.filename, parser);
        return result;
    }
    if (!ast)
    {
        files.diagnostics.push_back(
            FormulaFileDiagnostic{FormulaFileDiagnosticCode::PARSE_ERROR, file.filename, {}, {}, {}});
        return result;
    }

    if (entry.is_class)
    {
        if (const ClassHeader *header = find_class_header(file, entry_index))
        {
            result.references = collect_formula_references(*header, *ast);
            return result;
        }
    }

    result.references = collect_formula_references(*ast);
    return result;
}

void collect_formula_file_references(FormulaFileSet &files)
{
    files.entry_references.clear();
    const std::size_t file_count = files.files.size();
    for (std::size_t file_index = 0; file_index < file_count; ++file_index)
    {
        const std::size_t entry_count = files.files[file_index].entries.size();
        for (std::size_t entry_index = 0; entry_index < entry_count; ++entry_index)
        {
            files.entry_references.push_back(collect_formula_entry_references(files, file_index, entry_index));
        }
    }
}

} // namespace formula
