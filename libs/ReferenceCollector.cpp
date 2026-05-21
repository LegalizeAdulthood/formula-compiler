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
#include <optional>
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

    void visit(const ast::FunctionBlockNode &node) override
    {
        add_reference(FormulaReferenceKind::FUNCTION_RETURN, node.type());
        visit_expr(node.block());
    }

    void visit(const ast::HeadingBlockNode &node) override
    {
        visit_expr(node.block());
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
        add_reference(FormulaReferenceKind::PARAM_BLOCK, node.type());
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

bool same_identifier(std::string_view lhs, std::string_view rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < lhs.size(); ++index)
    {
        const char left = static_cast<char>(std::tolower(static_cast<unsigned char>(lhs[index])));
        const char right = static_cast<char>(std::tolower(static_cast<unsigned char>(rhs[index])));
        if (left != right)
        {
            return false;
        }
    }
    return true;
}

std::optional<std::size_t> find_file_index(const FormulaFileSet &files, std::string_view filename)
{
    for (const FormulaFileReference &file : files.file_index)
    {
        if (file.filename == filename)
        {
            return file.file_index;
        }
    }
    return {};
}

std::optional<FormulaClassReference> find_class_in_file(
    const FormulaFileSet &files, std::size_t file_index, std::string_view class_name)
{
    for (const FormulaClassReference &klass : files.class_index)
    {
        if (klass.file_index == file_index && same_identifier(klass.class_name, class_name))
        {
            return klass;
        }
    }
    return {};
}

const FormulaEntryImports *find_entry_imports(const FormulaFile &file, std::size_t entry_index)
{
    for (const FormulaEntryImports &entry_imports : file.entry_imports)
    {
        if (entry_imports.entry_index == entry_index)
        {
            return &entry_imports;
        }
    }
    return nullptr;
}

std::optional<FormulaClassReference> resolve_imported_class(
    const FormulaFileSet &files, const FormulaImportDirective &import, std::string_view class_name)
{
    const std::optional<std::size_t> imported_file_index = find_file_index(files, import.filename);
    if (!imported_file_index)
    {
        return {};
    }
    return find_class_in_file(files, *imported_file_index, class_name);
}

std::optional<FormulaClassReference> resolve_class_reference(
    const FormulaFileSet &files, const FormulaEntryReferences &entry_references, const FormulaReference &reference)
{
    if (entry_references.file_index >= files.files.size())
    {
        return {};
    }

    const FormulaFile &file = files.files[entry_references.file_index];
    if (const FormulaEntryImports *entry_imports = find_entry_imports(file, entry_references.entry_index))
    {
        for (auto it = entry_imports->imports.rbegin(); it != entry_imports->imports.rend(); ++it)
        {
            if (!it->implicit)
            {
                if (std::optional<FormulaClassReference> klass =
                        resolve_imported_class(files, *it, reference.class_name))
                {
                    return klass;
                }
            }
        }

        for (auto it = entry_imports->imports.rbegin(); it != entry_imports->imports.rend(); ++it)
        {
            if (it->implicit)
            {
                if (std::optional<FormulaClassReference> klass =
                        resolve_imported_class(files, *it, reference.class_name))
                {
                    return klass;
                }
            }
        }
    }

    return find_class_in_file(files, entry_references.file_index, reference.class_name);
}

void add_unresolved_diagnostic(
    FormulaFileSet &files, const FormulaEntryReferences &entry_references, const FormulaReference &reference)
{
    SourceLocation location{reference.location};
    if (location.filename.empty())
    {
        location.filename = entry_references.filename;
    }
    for (const FormulaFileDiagnostic &diagnostic : files.diagnostics)
    {
        if (diagnostic.code == FormulaFileDiagnosticCode::UNRESOLVED_CLASS &&
            diagnostic.filename == entry_references.filename && diagnostic.location.filename == location.filename &&
            diagnostic.location.line == location.line && diagnostic.location.column == location.column &&
            diagnostic.detail == reference.class_name)
        {
            return;
        }
    }
    files.diagnostics.push_back(FormulaFileDiagnostic{FormulaFileDiagnosticCode::UNRESOLVED_CLASS,
        entry_references.filename, std::move(location), reference.class_name, {}});
}

void clear_unresolved_diagnostics(FormulaFileSet &files)
{
    files.diagnostics.erase(
        std::remove_if(files.diagnostics.begin(), files.diagnostics.end(), [](const FormulaFileDiagnostic &diagnostic)
            { return diagnostic.code == FormulaFileDiagnosticCode::UNRESOLVED_CLASS; }),
        files.diagnostics.end());
}

bool same_resolved_reference(const FormulaResolvedReference &lhs, const FormulaResolvedReference &rhs)
{
    return lhs.entry.file_index == rhs.entry.file_index && lhs.entry.entry_index == rhs.entry.entry_index &&
        lhs.reference.kind == rhs.reference.kind && lhs.reference.class_name == rhs.reference.class_name &&
        lhs.reference.location.line == rhs.reference.location.line &&
        lhs.reference.location.column == rhs.reference.location.column &&
        lhs.klass.file_index == rhs.klass.file_index && lhs.klass.entry_index == rhs.klass.entry_index;
}

void add_resolved_reference(FormulaFileSet &files, FormulaResolvedReference reference)
{
    for (const FormulaResolvedReference &existing : files.resolved_references)
    {
        if (same_resolved_reference(existing, reference))
        {
            return;
        }
    }
    files.resolved_references.push_back(std::move(reference));
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

std::vector<FormulaResolvedReference> resolve_formula_entry_references(
    const FormulaFileSet &files, const FormulaEntryReferences &entry_references)
{
    std::vector<FormulaResolvedReference> result;
    for (const FormulaReference &reference : entry_references.references)
    {
        if (std::optional<FormulaClassReference> klass = resolve_class_reference(files, entry_references, reference))
        {
            result.push_back(FormulaResolvedReference{entry_references, reference, *klass});
        }
    }
    return result;
}

void resolve_formula_file_references(FormulaFileSet &files)
{
    files.resolved_references.clear();
    clear_unresolved_diagnostics(files);
    if (files.entry_references.empty())
    {
        collect_formula_file_references(files);
    }
    for (const FormulaEntryReferences &entry_references : files.entry_references)
    {
        for (const FormulaReference &reference : entry_references.references)
        {
            if (std::optional<FormulaClassReference> klass =
                    resolve_class_reference(files, entry_references, reference))
            {
                files.resolved_references.push_back(FormulaResolvedReference{entry_references, reference, *klass});
            }
            else
            {
                add_unresolved_diagnostic(files, entry_references, reference);
            }
        }
    }
}

void resolve_root_formula_file_references(FormulaFileSet &files)
{
    files.resolved_references.clear();
    clear_unresolved_diagnostics(files);
    if (files.entry_references.empty())
    {
        collect_formula_file_references(files);
    }
    for (const FormulaEntryReferences &entry_references : files.entry_references)
    {
        if (entry_references.file_index != 0)
        {
            continue;
        }
        for (const FormulaReference &reference : entry_references.references)
        {
            if (std::optional<FormulaClassReference> klass =
                    resolve_class_reference(files, entry_references, reference))
            {
                files.resolved_references.push_back(FormulaResolvedReference{entry_references, reference, *klass});
            }
            else
            {
                add_unresolved_diagnostic(files, entry_references, reference);
            }
        }
    }
}

void retain_resolved_imported_classes(FormulaFileSet &files)
{
    if (files.resolved_references.empty())
    {
        resolve_root_formula_file_references(files);
    }
    std::size_t retained_index{};
    for (const FormulaResolvedReference &reference : files.resolved_references)
    {
        if (reference.entry.file_index == 0 && reference.klass.file_index != reference.entry.file_index)
        {
            retain_formula_class(files, reference.klass);
        }
    }
    while (retained_index < files.retained_classes.size())
    {
        const RetainedFormulaClass retained{files.retained_classes[retained_index++]};
        FormulaEntryReferences entry_references;
        entry_references.file_index = retained.reference.file_index;
        entry_references.entry_index = retained.reference.entry_index;
        entry_references.filename = retained.reference.filename;
        if (const ClassHeader *header =
                find_class_header(files.files[retained.reference.file_index], retained.reference.entry_index))
        {
            entry_references.references = collect_formula_references(*header, *retained.ast);
        }
        else
        {
            entry_references.references = collect_formula_references(*retained.ast);
        }

        for (const FormulaReference &reference : entry_references.references)
        {
            if (std::optional<FormulaClassReference> klass =
                    resolve_class_reference(files, entry_references, reference))
            {
                add_resolved_reference(files, FormulaResolvedReference{entry_references, reference, *klass});
                if (klass->file_index != entry_references.file_index)
                {
                    retain_formula_class(files, *klass);
                }
            }
            else
            {
                add_unresolved_diagnostic(files, entry_references, reference);
            }
        }
    }
}

} // namespace formula
