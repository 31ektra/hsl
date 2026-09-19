#include "hsl/parser.hpp"

#include <string>
#include <utility>

namespace hsl {
namespace {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

    ParseResult parse() {
        auto program = node(AstKind::Program, current().span);
        newlines();
        while (!at(Kind::Eof)) {
            const std::size_t start = position_;

            if (at(Kind::Import)) {
                program->children.push_back(import_declaration());
            } else if (at(Kind::Fn)) {
                program->children.push_back(function());
            } else if (at(Kind::Struct)) {
                program->children.push_back(struct_declaration());
            } else if (at(Kind::Enum)) {
                program->children.push_back(enum_declaration());
            } else {
                error(
                    current(),
                    "expected function, struct, or enum declaration"
                );
                sync();
            }

            newlines();
            ensure_progress(start);
        }
        return {std::move(program), std::move(diagnostics_)};
    }

private:
    const Token& current() const { return tokens_[position_]; }
    const Token& previous() const { return tokens_[position_ - 1]; }
    bool at(Kind kind) const { return current().kind == kind; }
    bool ended() const { return at(Kind::Eof); }

    const Token& advance() {
        if (!ended()) ++position_;
        return previous();
    }

    bool match(Kind kind) {
        if (!at(kind)) return false;
        advance();
        return true;
    }

    void expect_generic_greater() {
        if (pending_generic_greater_ > 0) {
            --pending_generic_greater_;
            return;
        }
        if (match(Kind::Greater)) return;
        if (match(Kind::ShiftRight)) {
            pending_generic_greater_ = 1;
            return;
        }
        error(current(), "expected '>' after generic type arguments");
    }

    const Token& expect(Kind kind, std::string message) {
        if (at(kind)) return advance();
        error(current(), std::move(message));
        return current();
    }

    void error(const Token& token, std::string message) {
        diagnostics_.push_back({DiagnosticSeverity::Error, token.span, std::move(message)});
    }

    void newlines() { while (match(Kind::Newline)) {} }

    void sync() {
        const std::size_t start = position_;

        while (
            !ended() &&
            !at(Kind::Newline) &&
            !at(Kind::Dedent) &&
            !at(Kind::Semicolon)
        ) {
            advance();
        }

        if (
            at(Kind::Semicolon) ||
            at(Kind::Newline) ||
            at(Kind::Dedent)
        ) {
            advance();
        }

        ensure_progress(start);
    }

    void ensure_progress(std::size_t start) {
        if (position_ != start || ended()) {
            return;
        }

        error(
            current(),
            "parser could not recover from this token"
        );

        advance();
    }

    bool begin_continuation() {
        if (!at(Kind::Newline)) {
            return false;
        }

        newlines();

        if (match(Kind::Indent)) {
            ++continuation_depth_;
            newlines();
            return true;
        }

        return false;
    }

    void close_available_continuations() {
        newlines();

        while (
            continuation_depth_ > 0 &&
            match(Kind::Dedent)
        ) {
            --continuation_depth_;
            newlines();
        }
    }

    void finish_statement() {
        close_available_continuations();
    }

    static std::unique_ptr<AstNode> node(AstKind kind, SourceSpan span, std::string value = {}) {
        return std::make_unique<AstNode>(AstNode{kind, span, std::move(value), {}});
    }

    std::string type_name_parse() {
        if (match(Kind::Amp)) {
            return "&" + type_name_parse();
        }

        if (match(Kind::LBracket)) {
            std::string result = "[";
            result += type_name_parse();

            if (match(Kind::RBracket)) {
                result += ']';
                return result;
            }

            expect(
                Kind::Semicolon,
                "expected ';' after array element type"
            );

            result += "; ";

            const auto& length = expect(
                Kind::Integer,
                "expected array length"
            );

            result += length.text;

            expect(
                Kind::RBracket,
                "expected ']' after array type"
            );

            result += ']';
            return result;
        }

        if (!type(current().kind) && !at(Kind::Identifier)) {
            error(current(), "expected type name");
            return {};
        }

        std::string result(current().text);
        const bool primitive = type(current().kind);
        advance();

        if (!primitive && match(Kind::Less)) {
            result += '<';
            while (true) {
                result += type_name_parse();
                if (!match(Kind::Comma)) break;
                result += ',';
            }
            expect_generic_greater();
            result += '>';
            return result;
        }

        if (primitive || !match(Kind::Dot)) {
            return result;
        }

        const auto& member = expect(
            Kind::Identifier,
            "expected type name after '.'"
        );

        result += '.';
        result += member.text;

        return result;
    }

    std::unique_ptr<AstNode> import_declaration() {
        const auto start = advance().span;

        const auto& module = expect(
            Kind::Identifier,
            "expected module name after 'import'"
        );

        auto result = node(
            AstKind::ImportDeclaration,
            start,
            std::string(module.text)
        );

        expect(
            Kind::Semicolon,
            "expected ';' after import declaration"
        );

        finish_statement();
        return result;
    }

    std::unique_ptr<AstNode> enum_declaration() {
        const auto start = advance().span;
        const auto& name = expect(
            Kind::Identifier,
            "expected enum name after 'enum'"
        );
        auto result = node(
            AstKind::EnumDeclaration,
            start,
            std::string(name.text)
        );
        expect(Kind::Colon, "expected ':' after enum name");
        expect(Kind::Newline, "expected newline after enum declaration");
        expect(Kind::Indent, "expected indented enum body");
        newlines();
        while (!at(Kind::Dedent) && !ended()) {
            const auto& variant = expect(
                Kind::Identifier,
                "expected enum variant name"
            );
            std::string variant_label(variant.text);
            if (match(Kind::LParen)) {
                variant_label += ':';
                variant_label += type_name_parse();
                expect(Kind::RParen, "expected ')' after enum variant payload type");
            }
            expect(Kind::Semicolon, "expected ';' after enum variant");
            result->children.push_back(node(
                AstKind::EnumVariant,
                variant.span,
                std::move(variant_label)
            ));
            newlines();
        }
        expect(Kind::Dedent, "expected end of enum body");
        return result;
    }

    std::unique_ptr<AstNode> struct_declaration() {
        const auto start = advance().span;
        const auto& name = expect(
            Kind::Identifier,
            "expected struct name after 'struct'"
        );

        auto result = node(
            AstKind::StructDeclaration,
            start,
            std::string(name.text)
        );

        expect(Kind::Colon, "expected ':' after struct name");
        expect(Kind::Newline, "expected newline after struct declaration");
        expect(Kind::Indent, "expected indented struct body");

        newlines();

        while (!at(Kind::Dedent) && !ended()) {
            const std::size_t field_start = position_;

            const auto& field_name = expect(
                Kind::Identifier,
                "expected field name"
            );

            expect(Kind::Colon, "expected ':' after field name");

            std::string field_label(field_name.text);
            field_label += ':';
            field_label += type_name_parse();

            expect(
                Kind::Semicolon,
                "expected ';' after field declaration"
            );

            result->children.push_back(node(
                AstKind::FieldDeclaration,
                field_name.span,
                std::move(field_label)
            ));

            newlines();
            ensure_progress(field_start);
        }

        expect(Kind::Dedent, "expected end of struct body");
        return result;
    }

    std::unique_ptr<AstNode> function() {
        const auto start = advance().span;
        const auto& name = expect(Kind::Identifier, "expected function name after 'fn'");
        auto result = node(AstKind::Function, start, std::string(name.text));
        expect(Kind::LParen, "expected '(' after function name");
        begin_continuation();
        if (!at(Kind::RParen)) {
            while (true) {
                const auto& parameter_name = expect(
                    Kind::Identifier,
                    "expected parameter name"
                );
                expect(Kind::Colon, "expected ':' after parameter name");

                std::string parameter_label(parameter_name.text);
                parameter_label += ':';
                parameter_label += type_name_parse();

                result->children.push_back(node(
                    AstKind::Parameter,
                    parameter_name.span,
                    std::move(parameter_label)
                ));

                if (!match(Kind::Comma)) {
                    break;
                }
                newlines();
            }
        }
        close_available_continuations();
        expect(Kind::RParen, "expected ')' after function parameters");
        if (match(Kind::Arrow)) {
            result->value += ':';
            result->value += type_name_parse();
        }
        expect(Kind::Colon, "expected ':' after function signature");
        expect(Kind::Newline, "expected newline after function signature");
        expect(Kind::Indent, "expected indented function body");

        auto body = node(AstKind::Block, current().span);
        newlines();
        while (!at(Kind::Dedent) && !ended()) {
            const std::size_t statement_start = position_;

            body->children.push_back(statement());
            newlines();
            close_available_continuations();
            ensure_progress(statement_start);
        }
        expect(Kind::Dedent, "expected end of function body");
        result->children.push_back(std::move(body));
        return result;
    }

    std::unique_ptr<AstNode> statement() {
        if (at(Kind::Let) || at(Kind::Var)) return variable();
        if (at(Kind::Return)) return return_statement();
        if (at(Kind::If)) return if_statement();
        if (at(Kind::Match)) return match_statement();
        if (at(Kind::While)) return while_statement();
        if (at(Kind::For)) return for_statement();
        if (at(Kind::Break)) return loop_control(AstKind::BreakStatement, "break");
        if (at(Kind::Continue)) return loop_control(AstKind::ContinueStatement, "continue");
        auto expression = expression_parse();
        auto result = node(AstKind::ExpressionStatement, expression->span);
        result->children.push_back(std::move(expression));
        expect(Kind::Semicolon, "expected ';' after expression");
        finish_statement();
        return result;
    }

    std::unique_ptr<AstNode> match_statement() {
        const auto start = advance().span;
        auto result = node(AstKind::MatchStatement, start);
        result->children.push_back(expression_parse());
        expect(Kind::Colon, "expected ':' after match value");
        expect(Kind::Newline, "expected newline after match value");
        expect(Kind::Indent, "expected indented match arms");
        newlines();
        while (!at(Kind::Dedent) && !ended()) {
            const auto arm_start = current().span;
            std::string pattern;
            if (at(Kind::Identifier) && current().text == "_") {
                pattern = "_";
                advance();
            } else {
                const auto& enum_name = expect(Kind::Identifier, "expected enum name in match pattern");
                expect(Kind::Dot, "expected '.' in enum pattern");
                const auto& variant = expect(Kind::Identifier, "expected enum variant in match pattern");
                pattern = std::string(enum_name.text) + "." + std::string(variant.text);
                if (match(Kind::LParen)) {
                    const auto& binding = expect(Kind::Identifier, "expected payload binding name");
                    pattern += ":" + std::string(binding.text);
                    expect(Kind::RParen, "expected ')' after payload binding");
                }
            }
            expect(Kind::Colon, "expected ':' after match pattern");
            auto arm = node(AstKind::MatchArm, arm_start, std::move(pattern));
            arm->children.push_back(parse_block("expected indented match arm body"));
            result->children.push_back(std::move(arm));
            newlines();
        }
        expect(Kind::Dedent, "expected end of match statement");
        return result;
    }

    std::unique_ptr<AstNode> variable() {
        const bool mutable_value = at(Kind::Var);
        const auto start = advance().span;
        const auto& name = expect(Kind::Identifier, "expected variable name");
        std::string label(name.text);
        if (match(Kind::Colon)) {
            label += ':';
            label += type_name_parse();
        }
        expect(
            Kind::Equal,
            "expected '=' in variable declaration"
        );

        begin_continuation();

        auto result = node(
            mutable_value
                ? AstKind::VarDeclaration
                : AstKind::LetDeclaration,
            start,
            std::move(label)
        );

        result->children.push_back(expression_parse());

        expect(
            Kind::Semicolon,
            "expected ';' after variable declaration"
        );

        finish_statement();
        return result;
    }

    std::unique_ptr<AstNode> parse_block(std::string message) {
        expect(Kind::Newline, "expected newline before block");
        expect(Kind::Indent, std::move(message));
        auto result = node(AstKind::Block, current().span);
        newlines();
        while (!at(Kind::Dedent) && !ended()) {
            const std::size_t statement_start = position_;

            result->children.push_back(statement());
            newlines();
            close_available_continuations();
            ensure_progress(statement_start);
        }

        expect(Kind::Dedent, "expected end of block");
        return result;
    }

    std::unique_ptr<AstNode> if_statement() {
        const auto start = advance().span;
        auto result = node(AstKind::IfStatement, start);
        result->children.push_back(expression_parse());
        expect(Kind::Colon, "expected ':' after if condition");
        result->children.push_back(parse_block("expected indented 'if' body"));

        if (match(Kind::Else)) {
            const auto else_span = previous().span;
            expect(Kind::Colon, "expected ':' after 'else'");
            auto alternative = node(AstKind::ElseClause, else_span);
            alternative->children.push_back(parse_block("expected indented 'else' body"));
            result->children.push_back(std::move(alternative));
        }
        return result;
    }

    std::unique_ptr<AstNode> for_statement() {
        const auto start = advance().span;
        const auto& iterator = expect(Kind::Identifier, "expected loop variable after 'for'");
        expect(Kind::In, "expected 'in' after loop variable");
        auto result = node(AstKind::ForStatement, start, std::string(iterator.text));
        result->children.push_back(expression_parse());
        expect(Kind::Colon, "expected ':' after for iterable");
        ++loop_depth_;
        result->children.push_back(parse_block("expected indented 'for' body"));
        --loop_depth_;
        return result;
    }

    std::unique_ptr<AstNode> while_statement() {
        const auto start = advance().span;
        auto result = node(AstKind::WhileStatement, start);
        result->children.push_back(expression_parse());
        expect(Kind::Colon, "expected ':' after while condition");
        ++loop_depth_;
        result->children.push_back(parse_block("expected indented 'while' body"));
        --loop_depth_;
        return result;
    }

    std::unique_ptr<AstNode> loop_control(AstKind kind, std::string_view keyword) {
        const auto start = advance().span;
        if (loop_depth_ == 0) {
            error(previous(), "'" + std::string(keyword) + "' can only be used inside a loop");
        }
        expect(
            Kind::Semicolon,
            "expected ';' after '" + std::string(keyword) + "'"
        );

        finish_statement();
        return node(kind, start);
    }

    std::unique_ptr<AstNode> return_statement() {
        const auto start = advance().span;
        auto result = node(AstKind::ReturnStatement, start);

        begin_continuation();

        if (!at(Kind::Semicolon)) {
            result->children.push_back(expression_parse());
        }

        expect(
            Kind::Semicolon,
            "expected ';' after return statement"
        );

        finish_statement();
        return result;
    }

    std::unique_ptr<AstNode> expression_parse(int minimum = 1) {
        auto left = unary();
        while (precedence(current().kind) >= minimum) {
            const auto operation = advance();
            const int level = precedence(operation.kind);

            begin_continuation();

            auto right = expression_parse(
                operation.kind == Kind::Power
                    ? level
                    : level + 1
            );
            auto result = node(AstKind::BinaryExpression, operation.span,
                               std::string(kind_name(operation.kind)));
            result->children.push_back(std::move(left));
            result->children.push_back(std::move(right));
            left = std::move(result);
        }
        if (minimum == 1 &&
            (at(Kind::RangeExclusive) || at(Kind::RangeInclusive))) {
            const auto operation = advance();

            begin_continuation();

            auto right = expression_parse();
            auto range = node(
                AstKind::RangeExpression,
                operation.span,
                operation.kind == Kind::RangeInclusive ? "Inclusive" : "Exclusive"
            );
            range->children.push_back(std::move(left));
            range->children.push_back(std::move(right));
            left = std::move(range);
        }
        if (minimum == 1 && assignment_operator(current().kind)) {
            const auto operation = advance();

            begin_continuation();

            auto right = expression_parse();
            auto assignment = node(
                AstKind::AssignmentExpression,
                operation.span,
                std::string(kind_name(operation.kind))
            );
            assignment->children.push_back(std::move(left));
            assignment->children.push_back(std::move(right));
            left = std::move(assignment);
        }
        return left;
    }

    static bool assignment_operator(Kind kind) {
        return kind == Kind::Equal || kind == Kind::PlusEqual ||
               kind == Kind::MinusEqual || kind == Kind::StarEqual ||
               kind == Kind::SlashEqual || kind == Kind::PercentEqual ||
               kind == Kind::AmpEqual || kind == Kind::PipeEqual ||
               kind == Kind::CaretEqual || kind == Kind::ShiftLeftEqual ||
               kind == Kind::ShiftRightEqual;
    }

    std::unique_ptr<AstNode> unary() {
        if (at(Kind::Move)) {
            const auto operation = advance();
            auto result = node(
                AstKind::MoveExpression,
                operation.span
            );
            result->children.push_back(unary());
            return result;
        }

        if (at(Kind::Amp)) {
            const auto operation = advance();
            auto result = node(
                AstKind::BorrowExpression,
                operation.span
            );
            result->children.push_back(unary());
            return result;
        }

        if (at(Kind::Star)) {
            const auto operation = advance();
            auto result = node(
                AstKind::DereferenceExpression,
                operation.span
            );
            result->children.push_back(unary());
            return result;
        }

        if (at(Kind::Bang) || at(Kind::Minus) || at(Kind::Plus) || at(Kind::Tilde)) {
            const auto operation = advance();
            auto result = node(AstKind::UnaryExpression, operation.span,
                               std::string(kind_name(operation.kind)));
            result->children.push_back(unary());
            return result;
        }
        return call();
    }

    std::unique_ptr<AstNode> call() {
        auto result = primary();

        while (true) {
            if (match(Kind::LParen)) {
                auto invocation = node(
                    AstKind::CallExpression,
                    previous().span
                );

                invocation->children.push_back(std::move(result));

                begin_continuation();

                if (!at(Kind::RParen)) {
                    while (true) {
                        invocation->children.push_back(
                            expression_parse()
                        );

                        close_available_continuations();

                        if (!match(Kind::Comma)) {
                            break;
                        }

                        begin_continuation();
                    }
                }

                close_available_continuations();

                expect(
                    Kind::RParen,
                    "expected ')' after arguments"
                );

                result = std::move(invocation);
                continue;
            }

            if (match(Kind::Dot)) {
                const auto& member_name = expect(
                    Kind::Identifier,
                    "expected member name after '.'"
                );

                auto member = node(
                    AstKind::MemberExpression,
                    member_name.span,
                    std::string(member_name.text)
                );

                member->children.push_back(std::move(result));
                result = std::move(member);
                continue;
            }

            if (match(Kind::LBracket)) {
                const auto bracket_span = previous().span;
                begin_continuation();

                if (
                    at(Kind::RangeExclusive) ||
                    at(Kind::RangeInclusive)
                ) {
                    const auto operation = advance();
                    const bool has_end = !at(Kind::RBracket);

                    auto slice = node(
                        AstKind::SliceExpression,
                        bracket_span,
                        operation.kind == Kind::RangeInclusive
                            ? (has_end ? "InclusiveEnd" : "InclusiveFull")
                            : (has_end ? "ExclusiveEnd" : "Full")
                    );

                    slice->children.push_back(std::move(result));

                    if (has_end) {
                        slice->children.push_back(
                            expression_parse(2)
                        );
                    }

                    close_available_continuations();

                    expect(
                        Kind::RBracket,
                        "expected ']' after slice"
                    );

                    result = std::move(slice);
                    continue;
                }

                auto first = expression_parse(2);

                if (
                    at(Kind::RangeExclusive) ||
                    at(Kind::RangeInclusive)
                ) {
                    const auto operation = advance();
                    const bool has_end = !at(Kind::RBracket);

                    auto slice = node(
                        AstKind::SliceExpression,
                        bracket_span,
                        operation.kind == Kind::RangeInclusive
                            ? (has_end ? "InclusiveBoth" : "InclusiveStart")
                            : (has_end ? "ExclusiveBoth" : "ExclusiveStart")
                    );

                    slice->children.push_back(std::move(result));
                    slice->children.push_back(std::move(first));

                    if (has_end) {
                        slice->children.push_back(
                            expression_parse(2)
                        );
                    }

                    close_available_continuations();

                    expect(
                        Kind::RBracket,
                        "expected ']' after slice"
                    );

                    result = std::move(slice);
                    continue;
                }

                auto index = node(
                    AstKind::IndexExpression,
                    bracket_span
                );

                index->children.push_back(std::move(result));
                index->children.push_back(std::move(first));

                close_available_continuations();

                expect(
                    Kind::RBracket,
                    "expected ']' after array index"
                );

                result = std::move(index);
                continue;
            }

            break;
        }

        return result;
    }

    std::unique_ptr<AstNode> primary() {
        const auto token = advance();
        switch (token.kind) {
            case Kind::Identifier: {
                std::string name(token.text);
                if (
                    (name == "List" || name == "Option" || name == "Result" || name == "Map" || name == "Set") &&
                    match(Kind::Less)
                ) {
                    name += '<';
                    while (true) {
                        name += type_name_parse();
                        if (!match(Kind::Comma)) break;
                        name += ',';
                    }
                    expect_generic_greater();
                    name += '>';
                }
                return node(AstKind::IdentifierExpression, token.span, std::move(name));
            }
            case Kind::Integer: return node(AstKind::IntegerLiteral, token.span, std::string(token.text));
            case Kind::Float: return node(AstKind::FloatLiteral, token.span, std::string(token.text));
            case Kind::String: return node(AstKind::StringLiteral, token.span, std::string(token.text));
            case Kind::True:
            case Kind::False: return node(AstKind::BooleanLiteral, token.span, std::string(token.text));
            case Kind::LBracket: {
                auto result = node(
                    AstKind::ArrayExpression,
                    token.span
                );

                begin_continuation();

                if (!at(Kind::RBracket)) {
                    while (true) {
                        result->children.push_back(
                            expression_parse()
                        );

                        close_available_continuations();

                        if (!match(Kind::Comma)) {
                            break;
                        }

                        begin_continuation();
                    }
                }

                close_available_continuations();

                expect(
                    Kind::RBracket,
                    "expected ']' after array literal"
                );

                return result;
            }
            case Kind::LParen: {
                begin_continuation();

                auto result = expression_parse();

                close_available_continuations();

                expect(
                    Kind::RParen,
                    "expected ')' after expression"
                );

                return result;
            }
            default:
                error(token, "expected expression");
                return node(AstKind::IdentifierExpression, token.span, "<error>");
        }
    }

    static bool type(Kind kind) {
        return kind == Kind::TypeI8 || kind == Kind::TypeU8 ||
               kind == Kind::TypeI16 || kind == Kind::TypeU16 ||
               kind == Kind::TypeI32 || kind == Kind::TypeU32 ||
               kind == Kind::TypeI64 || kind == Kind::TypeU64 ||
               kind == Kind::TypeI128 || kind == Kind::TypeU128 ||
               kind == Kind::TypeISize || kind == Kind::TypeUSize ||
               kind == Kind::TypeF32 || kind == Kind::TypeF64 ||
               kind == Kind::TypeBool || kind == Kind::TypeStr ||
               kind == Kind::TypeNever;
    }

    static int precedence(Kind kind) {
        switch (kind) {
            case Kind::PipePipe: return 1;
            case Kind::AmpAmp: return 2;
            case Kind::Pipe: return 3;
            case Kind::Caret: return 4;
            case Kind::Amp: return 5;
            case Kind::EqualEqual:
            case Kind::BangEqual: return 6;
            case Kind::Less:
            case Kind::LessEqual:
            case Kind::Greater:
            case Kind::GreaterEqual: return 7;
            case Kind::ShiftLeft:
            case Kind::ShiftRight: return 8;
            case Kind::Plus:
            case Kind::Minus: return 9;
            case Kind::Star:
            case Kind::Slash:
            case Kind::Percent: return 10;
            case Kind::Power: return 11;
            default: return 0;
        }
    }

    const std::vector<Token>& tokens_;
    std::size_t position_ = 0;
    std::size_t pending_generic_greater_ = 0;
    std::vector<Diagnostic> diagnostics_;
    std::size_t loop_depth_ = 0;
    std::size_t continuation_depth_ = 0;
};

}

bool ParseResult::failed() const noexcept { return !diagnostics.empty(); }

ParseResult parse_tokens(const std::vector<Token>& tokens) {
    return Parser(tokens).parse();
}

}
