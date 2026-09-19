#include "hsl/ast.hpp"

#include <iostream>

namespace hsl {

std::string_view ast_kind_name(AstKind kind) {
    switch (kind) {
        case AstKind::Program: return "Program";
        case AstKind::ImportDeclaration: return "Import";
        case AstKind::StructDeclaration: return "Struct";
        case AstKind::EnumDeclaration: return "Enum";
        case AstKind::EnumVariant: return "Variant";
        case AstKind::FieldDeclaration: return "Field";
        case AstKind::Function: return "Function";
        case AstKind::Parameter: return "Parameter";
        case AstKind::Block: return "Block";
        case AstKind::IfStatement: return "If";
        case AstKind::ElseClause: return "Else";
        case AstKind::MatchStatement: return "Match";
        case AstKind::MatchArm: return "Arm";
        case AstKind::WhileStatement: return "While";
        case AstKind::ForStatement: return "For";
        case AstKind::RangeExpression: return "Range";
        case AstKind::BreakStatement: return "Break";
        case AstKind::ContinueStatement: return "Continue";
        case AstKind::LetDeclaration: return "Let";
        case AstKind::VarDeclaration: return "Var";
        case AstKind::ReturnStatement: return "Return";
        case AstKind::ExpressionStatement: return "ExpressionStatement";
        case AstKind::CallExpression: return "Call";
        case AstKind::MemberExpression: return "Member";
        case AstKind::IndexExpression: return "Index";
        case AstKind::SliceExpression: return "Slice";
        case AstKind::ArrayExpression: return "Array";
        case AstKind::AssignmentExpression: return "Assignment";
        case AstKind::BinaryExpression: return "Binary";
        case AstKind::UnaryExpression: return "Unary";
        case AstKind::BorrowExpression: return "Borrow";
        case AstKind::DereferenceExpression: return "Dereference";
        case AstKind::MoveExpression: return "Move";
        case AstKind::IdentifierExpression: return "Identifier";
        case AstKind::IntegerLiteral: return "Integer";
        case AstKind::FloatLiteral: return "Float";
        case AstKind::StringLiteral: return "String";
        case AstKind::BooleanLiteral: return "Boolean";
    }

    return "Unknown";
}

void print_ast(const AstNode& node, std::string_view prefix, bool last) {
    std::cout << prefix;

    if (!prefix.empty()) {
        std::cout << (last ? "`-- " : "|-- ");
    }

    std::cout << ast_kind_name(node.kind);

    if (!node.value.empty()) {
        std::cout << ' ' << node.value;
    }

    std::cout << '\n';

    const std::string child_prefix = std::string(prefix) +
        (prefix.empty() ? "" : (last ? "    " : "|   "));

    for (std::size_t index = 0; index < node.children.size(); ++index) {
        print_ast(
            *node.children[index],
            child_prefix.empty() ? "  " : child_prefix,
            index + 1 == node.children.size()
        );
    }
}

}
