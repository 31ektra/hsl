#pragma once

#include "hsl/source_span.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hsl {

enum class AstKind {
    Program,
    ImportDeclaration,
    StructDeclaration,
    EnumDeclaration,
    EnumVariant,
    FieldDeclaration,
    Function,
    Parameter,
    Block,
    IfStatement,
    ElseClause,
    MatchStatement,
    MatchArm,
    WhileStatement,
    ForStatement,
    RangeExpression,
    BreakStatement,
    ContinueStatement,
    LetDeclaration,
    VarDeclaration,
    ReturnStatement,
    ExpressionStatement,
    CallExpression,
    MemberExpression,
    IndexExpression,
    SliceExpression,
    ArrayExpression,
    AssignmentExpression,
    BinaryExpression,
    UnaryExpression,
    BorrowExpression,
    DereferenceExpression,
    MoveExpression,
    IdentifierExpression,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    BooleanLiteral,
};

struct AstNode {
    AstKind kind;
    SourceSpan span;
    std::string value;
    std::vector<std::unique_ptr<AstNode>> children;
};

std::string_view ast_kind_name(AstKind kind);
void print_ast(const AstNode& node, std::string_view prefix = {}, bool last = true);

}
