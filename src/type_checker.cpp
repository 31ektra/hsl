#include "hsl/type_checker.hpp"

#include "hsl/type.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace hsl {
namespace {

struct FunctionType {
    std::vector<Type> parameters;
    Type result{TypeKind::Void, {}};
};

struct StructField {
    std::string name;
    Type type;
};

struct StructType {
    std::vector<StructField> fields;
};

class TypeChecker {
public:
    TypeCheckResult check(const AstNode& root) {
        collect_structs(root);
        collect_enums(root);
        collect_functions(root);
        for (const auto& child : root.children) {
            if (child->kind == AstKind::Function) check_function(*child);
        }
        return {std::move(diagnostics_)};
    }

private:
    static std::string_view name_part(std::string_view label) {
        const auto separator = label.find(':');
        return label.substr(0, separator);
    }

    static std::string_view type_part(std::string_view label) {
        const auto separator = label.find(':');
        if (separator == std::string_view::npos) return {};
        return label.substr(separator + 1);
    }

    void report(const AstNode& node, std::string message) {
        diagnostics_.push_back({DiagnosticSeverity::Error, node.span, std::move(message)});
    }

    static bool contains_reference(const Type& type) {
        if (type.kind == TypeKind::Reference) {
            return true;
        }

        for (const auto& argument : type.arguments) {
            if (contains_reference(argument)) {
                return true;
            }
        }

        return false;
    }

    void validate_reference_container(
        const AstNode& owner,
        const Type& type
    ) {
        if (
            type.kind == TypeKind::Array &&
            !type.arguments.empty() &&
            contains_reference(type.arguments.front())
        ) {
            report(
                owner,
                "arrays cannot contain reference elements"
            );
        }

        if (
            type.kind == TypeKind::Slice &&
            !type.arguments.empty() &&
            contains_reference(type.arguments.front())
        ) {
            report(
                owner,
                "slices cannot contain reference elements"
            );
        }

        if (
            type.kind == TypeKind::List &&
            !type.arguments.empty() &&
            contains_reference(type.arguments.front())
        ) {
            report(
                owner,
                "lists cannot contain reference elements"
            );
        }
    }

    void collect_enums(const AstNode& root) {
        for (const auto& child : root.children) {
            if (child->kind != AstKind::EnumDeclaration) continue;
            auto& variants = enums_[child->value];
            for (const auto& variant : child->children) {
                variants[std::string(name_part(variant->value))] =
                    type_part(variant->value).empty()
                        ? Type{TypeKind::Void, {}}
                        : parse_type_name(type_part(variant->value));
            }
        }
    }

    void collect_structs(const AstNode& root) {
        for (const auto& child : root.children) {
            if (child->kind != AstKind::StructDeclaration) {
                continue;
            }

            StructType structure;

            for (const auto& field : child->children) {
                if (field->kind != AstKind::FieldDeclaration) {
                    continue;
                }

                const Type field_type =
                    parse_type_name(type_part(field->value));

                if (contains_reference(field_type)) {
                    report(
                        *field,
                        "struct fields cannot have reference types"
                    );
                }

                structure.fields.push_back({
                    std::string(name_part(field->value)),
                    field_type
                });
            }

            structs_[child->value] = std::move(structure);
        }
    }

    void collect_functions(const AstNode& root) {
        for (const auto& child : root.children) {
            if (child->kind != AstKind::Function) continue;
            FunctionType function;
            function.result =
                parse_type_name(type_part(child->value));

            if (contains_reference(function.result)) {
                report(
                    *child,
                    "functions cannot return references"
                );
            }

            for (const auto& item : child->children) {
                if (item->kind == AstKind::Parameter) {
                    const Type parameter =
                        parse_type_name(type_part(item->value));

                    validate_reference_container(
                        *item,
                        parameter
                    );

                    function.parameters.push_back(parameter);
                }
            }
            functions_[std::string(name_part(child->value))] = std::move(function);
        }
    }

    void check_function(const AstNode& function) {
        scopes_.clear();
        scopes_.emplace_back();
        current_function_ = std::string(name_part(function.value));
        current_return_ = parse_type_name(type_part(function.value));

        for (const auto& child : function.children) {
            if (child->kind == AstKind::Parameter) {
                scopes_.back()[std::string(name_part(child->value))] =
                    parse_type_name(type_part(child->value));
            }
        }

        for (const auto& child : function.children) {
            if (child->kind == AstKind::Block) check_block(*child, false);
        }
    }

    void check_block(const AstNode& block, bool nested = true) {
        if (nested) scopes_.emplace_back();
        for (const auto& statement : block.children) check_statement(*statement);
        if (nested) scopes_.pop_back();
    }

    Type variable_type(std::string_view name) const {
        for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
            const auto item = scope->find(std::string(name));
            if (item != scope->end()) return item->second;
        }
        return {TypeKind::Error, {}};
    }

    void check_statement(const AstNode& node) {
        switch (node.kind) {
            case AstKind::LetDeclaration:
            case AstKind::VarDeclaration: {
                const Type declared = parse_type_name(type_part(node.value));

                validate_reference_container(node, declared);

                const Type initializer = node.children.empty()
                    ? Type{TypeKind::Error, {}} : expression(*node.children.front());

                if (
                    declared.kind == TypeKind::List &&
                    !node.children.empty() &&
                    node.children.front()->kind == AstKind::IdentifierExpression
                ) {
                    report(
                        node,
                        "owned values require an explicit move"
                    );
                }
                if (!is_assignable_type(declared, initializer)) {
                    report(node, "cannot initialize '" + std::string(name_part(node.value)) +
                        "' of type " + std::string(type_name(declared)) + " with " +
                        std::string(type_name(initializer)));
                }
                scopes_.back()[std::string(name_part(node.value))] = declared;
                return;
            }
            case AstKind::MatchStatement: {
                if (node.children.empty()) return;
                const Type subject = expression(*node.children.front());
                if (subject.kind != TypeKind::UserDefined || !enums_.contains(subject.name)) {
                    report(node, "match requires an enum value");
                    return;
                }
                std::unordered_set<std::string> covered;
                bool wildcard = false;
                for (std::size_t index = 1; index < node.children.size(); ++index) {
                    const auto& arm = *node.children[index];
                    const auto separator = arm.value.find(':');
                    const std::string pattern = arm.value.substr(0, separator);
                    const std::string binding = separator == std::string::npos ? "" : arm.value.substr(separator + 1);
                    scopes_.emplace_back();
                    if (pattern == "_") {
                        if (wildcard) report(arm, "duplicate wildcard match arm");
                        wildcard = true;
                    } else {
                        const auto dot = pattern.find('.');
                        const std::string enum_name = pattern.substr(0, dot);
                        const std::string variant_name = dot == std::string::npos ? "" : pattern.substr(dot + 1);
                        if (enum_name != subject.name || !enums_.at(subject.name).contains(variant_name)) {
                            report(arm, "unknown enum variant in match pattern: " + pattern);
                        } else {
                            if (!covered.insert(variant_name).second) report(arm, "duplicate match arm: " + pattern);
                            const Type payload = enums_.at(subject.name).at(variant_name);
                            if (!binding.empty()) {
                                if (payload.kind == TypeKind::Void) report(arm, "fieldless variant cannot bind a payload");
                                else scopes_.back()[binding] = payload;
                            } else if (payload.kind != TypeKind::Void) {
                                report(arm, "payload variant requires a binding");
                            }
                        }
                    }
                    if (!arm.children.empty()) check_block(*arm.children.front(), false);
                    scopes_.pop_back();
                }
                if (!wildcard && covered.size() != enums_.at(subject.name).size()) {
                    report(node, "non-exhaustive match for enum " + subject.name);
                }
                return;
            }
            case AstKind::ForStatement: {
                if (node.children.empty()) {
                    report(
                        node,
                        "for loop requires a range or array expression"
                    );
                    return;
                }

                const auto& iterable = *node.children.front();
                Type iterator_type{TypeKind::Error, {}};

                if (
                    iterable.kind == AstKind::RangeExpression &&
                    iterable.children.size() == 2
                ) {
                    const Type start =
                        expression(*iterable.children[0]);
                    const Type end =
                        expression(*iterable.children[1]);

                    if (
                        start.kind != TypeKind::I64 ||
                        end.kind != TypeKind::I64
                    ) {
                        report(
                            iterable,
                            "range boundaries must have type i64"
                        );
                    }

                    iterator_type = Type{TypeKind::I64, {}};
                } else {
                    const Type iterable_type = expression(iterable);

                    if (
                        (iterable_type.kind == TypeKind::Array ||
                         iterable_type.kind == TypeKind::Slice ||
                         iterable_type.kind == TypeKind::List) &&
                        iterable_type.arguments.size() == 1
                    ) {
                        iterator_type =
                            iterable_type.arguments.front();
                    } else if (
                        iterable_type.kind != TypeKind::Error
                    ) {
                        report(
                            iterable,
                            "for loop requires a range or array expression"
                        );
                    }
                }

                scopes_.emplace_back();
                scopes_.back()[node.value] = iterator_type;

                if (node.children.size() > 1) {
                    check_block(*node.children[1], false);
                }

                scopes_.pop_back();
                return;
            }
            case AstKind::ReturnStatement: {
                const Type returned = node.children.empty()
                    ? Type{TypeKind::Void, {}} : expression(*node.children.front());
                if (
                    current_return_.kind == TypeKind::List &&
                    !node.children.empty() &&
                    node.children.front()->kind != AstKind::MoveExpression &&
                    node.children.front()->kind != AstKind::CallExpression
                ) {
                    report(
                        *node.children.front(),
                        "returning an owned value requires an explicit move"
                    );
                }
                if (!is_assignable_type(current_return_, returned)) {
                    report(node, "function '" + current_function_ + "' must return " +
                        std::string(type_name(current_return_)) + ", not " +
                        std::string(type_name(returned)));
                }
                return;
            }
            case AstKind::ExpressionStatement:
                for (const auto& child : node.children) expression(*child);
                return;
            case AstKind::IfStatement:
            case AstKind::WhileStatement:
                if (!node.children.empty()) {
                    const Type condition = expression(*node.children.front());
                    if (condition.kind != TypeKind::Bool && condition.kind != TypeKind::Error) {
                        report(*node.children.front(),
                            node.kind == AstKind::IfStatement
                                ? "if condition must have type bool"
                                : "while condition must have type bool");
                    }
                }
                for (std::size_t index = 1; index < node.children.size(); ++index) {
                    const auto& child = *node.children[index];
                    if (child.kind == AstKind::Block) check_block(child);
                    else if (child.kind == AstKind::ElseClause && !child.children.empty()) {
                        check_block(*child.children.front());
                    }
                }
                return;
            default:
                expression(node);
                return;
        }
    }

    Type expression(const AstNode& node) {
        switch (node.kind) {
            case AstKind::IdentifierExpression:
                return variable_type(node.value);
            case AstKind::IntegerLiteral:
                return {TypeKind::I64, {}};
            case AstKind::FloatLiteral:
                return node.value.ends_with("f32")
                    ? Type{TypeKind::F32, {}}
                    : Type{TypeKind::F64, {}};
            case AstKind::StringLiteral:
                return {TypeKind::Str, {}};
            case AstKind::BooleanLiteral:
                return {TypeKind::Bool, {}};
            case AstKind::RangeExpression:
                report(node, "range expression is only valid in a for loop");
                return {TypeKind::Error, {}};
            case AstKind::CallExpression:
                return call_expression(node);
            case AstKind::MemberExpression:
                return member_expression(node);
            case AstKind::IndexExpression:
                return index_expression(node);
            case AstKind::SliceExpression:
                return slice_expression(node);
            case AstKind::ArrayExpression:
                return array_expression(node);
            case AstKind::AssignmentExpression:
                return assignment_expression(node);
            case AstKind::UnaryExpression:
                return unary_expression(node);
            case AstKind::BorrowExpression:
                return borrow_expression(node);
            case AstKind::DereferenceExpression:
                return dereference_expression(node);
            case AstKind::MoveExpression:
                return move_expression(node);
            case AstKind::BinaryExpression:
                return binary_expression(node);
            default:
                return {TypeKind::Error, {}};
        }
    }

    Type array_expression(const AstNode& node) {
        if (node.children.empty()) {
            report(node, "cannot infer type of empty array");
            return {
                TypeKind::Array,
                {},
                {Type{TypeKind::Unknown, {}}},
                0
            };
        }

        const Type element = expression(*node.children.front());

        for (
            std::size_t index = 1;
            index < node.children.size();
            ++index
        ) {
            const Type received = expression(*node.children[index]);

            if (!is_assignable_type(element, received)) {
                report(
                    *node.children[index],
                    "array elements must have matching types"
                );
            }
        }

        return {
            TypeKind::Array,
            {},
            {element},
            node.children.size()
        };
    }

    Type slice_expression(const AstNode& node) {
        if (node.children.empty()) {
            return {TypeKind::Error, {}};
        }

        const AstNode& base_node = *node.children.front();
        const Type base = expression(base_node);

        if (
            base.kind != TypeKind::Array &&
            base.kind != TypeKind::Slice
        ) {
            if (base.kind != TypeKind::Error) {
                report(
                    node,
                    "slicing requires an array or slice value"
                );
            }

            return {TypeKind::Error, {}};
        }

        if (base.arguments.size() != 1) {
            return {TypeKind::Error, {}};
        }

        if (
            base_node.kind == AstKind::ArrayExpression ||
            base_node.kind == AstKind::CallExpression
        ) {
            report(
                base_node,
                "cannot create a slice from a temporary value"
            );
        }

        for (
            std::size_t index = 1;
            index < node.children.size();
            ++index
        ) {
            const Type boundary = expression(*node.children[index]);

            if (
                boundary.kind != TypeKind::I64 &&
                boundary.kind != TypeKind::Error
            ) {
                report(
                    *node.children[index],
                    "slice boundaries must have type i64"
                );
            }
        }

        auto literal_value = [](const AstNode& boundary)
            -> std::optional<std::size_t> {
            if (boundary.kind != AstKind::IntegerLiteral) {
                return std::nullopt;
            }

            std::string spelling = boundary.value;
            spelling.erase(
                std::remove(
                    spelling.begin(),
                    spelling.end(),
                    '_'
                ),
                spelling.end()
            );

            try {
                return static_cast<std::size_t>(
                    std::stoull(spelling)
                );
            } catch (const std::exception&) {
                return std::nullopt;
            }
        };

        const bool has_start =
            node.value == "ExclusiveStart" ||
            node.value == "InclusiveStart" ||
            node.value == "ExclusiveBoth" ||
            node.value == "InclusiveBoth";

        const bool has_end =
            node.value == "ExclusiveEnd" ||
            node.value == "InclusiveEnd" ||
            node.value == "ExclusiveBoth" ||
            node.value == "InclusiveBoth";

        std::optional<std::size_t> start;
        std::optional<std::size_t> end;

        if (has_start && node.children.size() >= 2) {
            start = literal_value(*node.children[1]);
        }

        if (has_end) {
            const std::size_t end_index = has_start ? 2 : 1;

            if (node.children.size() > end_index) {
                end = literal_value(*node.children[end_index]);
            }
        }

        if (start && end && *start > *end) {
            report(
                node,
                "slice start must not exceed slice end"
            );
        }

        if (base.kind == TypeKind::Array && end) {
            const bool inclusive =
                node.value == "InclusiveEnd" ||
                node.value == "InclusiveBoth";
            const std::size_t maximum = inclusive
                ? base.length - (base.length != 0 ? 1 : 0)
                : base.length;

            if (*end > maximum) {
                report(
                    node,
                    "slice end " + std::to_string(*end) +
                        " is out of bounds for length " +
                        std::to_string(base.length)
                );
            }
        }

        return {
            TypeKind::Slice,
            {},
            {base.arguments.front()},
            0
        };
    }

    Type index_expression(const AstNode& node) {
        if (node.children.size() != 2) {
            return {TypeKind::Error, {}};
        }

        const Type base = expression(*node.children.front());
        const Type index = expression(*node.children.back());

        if (index.kind != TypeKind::I64 &&
            index.kind != TypeKind::Error) {
            report(
                *node.children.back(),
                "array index must have type i64"
            );
        }

        if (
            (base.kind != TypeKind::Array &&
             base.kind != TypeKind::Slice &&
             base.kind != TypeKind::List) ||
            base.arguments.size() != 1
        ) {
            if (base.kind != TypeKind::Error) {
                report(node, "indexing requires an array, slice, or list value");
            }

            return {TypeKind::Error, {}};
        }

        if (
            base.kind == TypeKind::Array &&
            node.children.back()->kind == AstKind::IntegerLiteral
        ) {
            std::string spelling = node.children.back()->value;
            spelling.erase(
                std::remove(spelling.begin(), spelling.end(), '_'),
                spelling.end()
            );

            try {
                const std::size_t literal =
                    static_cast<std::size_t>(std::stoull(spelling));

                if (literal >= base.length) {
                    report(
                        *node.children.back(),
                        "array index " + std::to_string(literal) +
                            " is out of bounds for length " +
                            std::to_string(base.length)
                    );
                }
            } catch (const std::exception&) {
                // Invalid integer spellings are diagnosed by the lexer.
            }
        }

        return base.arguments.front();
    }

    Type member_expression(const AstNode& node) {
        if (node.children.empty()) {
            return {TypeKind::Error, {}};
        }

        const auto& base_node = *node.children.front();
        if (
            base_node.kind == AstKind::IdentifierExpression &&
            enums_.contains(base_node.value)
        ) {
            if (!enums_.at(base_node.value).contains(node.value)) {
                report(node, "enum '" + base_node.value + "' has no variant named '" + node.value + "'");
                return {TypeKind::Error, {}};
            }
            return {TypeKind::UserDefined, base_node.value};
        }

        Type base = expression(base_node);

        if (base.kind == TypeKind::Reference && base.arguments.size() == 1) {
            base = base.arguments.front();
        }

        if (base.kind != TypeKind::UserDefined) {
            report(
                node,
                "member access requires a struct value"
            );
            return {TypeKind::Error, {}};
        }

        const auto structure = structs_.find(base.name);

        if (structure == structs_.end()) {
            report(
                node,
                "unknown struct type: " + base.name
            );
            return {TypeKind::Error, {}};
        }

        for (const auto& field : structure->second.fields) {
            if (field.name == node.value) {
                return field.type;
            }
        }

        report(
            node,
            "struct '" + base.name +
                "' has no field named '" + node.value + "'"
        );

        return {TypeKind::Error, {}};
    }

    Type struct_construction(
        const AstNode& node,
        const std::string& struct_name
    ) {
        const auto structure = structs_.find(struct_name);

        if (structure == structs_.end()) {
            return {TypeKind::Error, {}};
        }

        const std::size_t supplied = node.children.size() - 1;
        const std::size_t expected = structure->second.fields.size();

        if (supplied != expected) {
            report(
                node,
                "struct '" + struct_name + "' expects " +
                    std::to_string(expected) +
                    " field values but received " +
                    std::to_string(supplied)
            );
        }

        const std::size_t count =
            supplied < expected ? supplied : expected;

        for (std::size_t index = 0; index < count; ++index) {
            const Type received =
                expression(*node.children[index + 1]);

            const Type& field_type =
                structure->second.fields[index].type;

            if (!is_assignable_type(field_type, received)) {
                report(
                    *node.children[index + 1],
                    "field " + std::to_string(index + 1) +
                        " of '" + struct_name +
                        "' expects " + type_name(field_type) +
                        " but received " + type_name(received)
                );
            }
        }

        return {TypeKind::UserDefined, struct_name};
    }

    Type call_expression(const AstNode& node) {
        if (node.children.empty()) return {TypeKind::Error, {}};
        const auto& callee = *node.children.front();

        if (callee.kind == AstKind::MemberExpression) {
            if (callee.children.empty()) return {TypeKind::Error, {}};
            const auto& enum_base = *callee.children.front();
            if (enum_base.kind == AstKind::IdentifierExpression) {
                const Type generic = parse_type_name(enum_base.value);
                if (generic.kind == TypeKind::UserDefined && generic.name == "Option" && generic.arguments.size() == 1) {
                    const std::size_t supplied = node.children.size() - 1;
                    const std::size_t expected = callee.value == "Some" ? 1 : callee.value == "None" ? 0 : 99;
                    if (expected == 99) report(callee, "Option has no variant named '" + callee.value + "'");
                    else if (supplied != expected) report(node, "Option." + callee.value + " expects " + std::to_string(expected) + " payload values");
                    else if (expected == 1) {
                        const Type received = expression(*node.children[1]);
                        if (!is_assignable_type(generic.arguments[0], received)) report(*node.children[1], "Option payload expects " + type_name(generic.arguments[0]));
                    }
                    return generic;
                }
                if (generic.kind == TypeKind::UserDefined && generic.name == "Result" && generic.arguments.size() == 2) {
                    const std::size_t supplied = node.children.size() - 1;
                    const std::size_t which = callee.value == "Ok" ? 0 : callee.value == "Err" ? 1 : 99;
                    if (which == 99) report(callee, "Result has no variant named '" + callee.value + "'");
                    else if (supplied != 1) report(node, "Result." + callee.value + " expects 1 payload value");
                    else {
                        const Type received = expression(*node.children[1]);
                        if (!is_assignable_type(generic.arguments[which], received)) report(*node.children[1], "Result payload expects " + type_name(generic.arguments[which]));
                    }
                    return generic;
                }
            }
            if (
                enum_base.kind == AstKind::IdentifierExpression &&
                enums_.contains(enum_base.value)
            ) {
                const auto variant = enums_.at(enum_base.value).find(callee.value);
                if (variant == enums_.at(enum_base.value).end()) {
                    report(callee, "enum '" + enum_base.value + "' has no variant named '" + callee.value + "'");
                    return {TypeKind::Error, {}};
                }
                const std::size_t supplied = node.children.size() - 1;
                const bool has_payload = variant->second.kind != TypeKind::Void;
                const std::size_t expected = has_payload ? 1 : 0;
                if (supplied != expected) {
                    report(node, "enum variant '" + enum_base.value + "." + callee.value + "' expects " + std::to_string(expected) + " payload values");
                } else if (has_payload) {
                    const Type received = expression(*node.children[1]);
                    if (!is_assignable_type(variant->second, received)) {
                        report(*node.children[1], "enum variant payload expects " + type_name(variant->second) + " but received " + type_name(received));
                    }
                }
                return {TypeKind::UserDefined, enum_base.value};
            }
            const Type receiver = expression(enum_base);
            const std::size_t supplied = node.children.size() - 1;

            if (receiver.kind == TypeKind::UserDefined && receiver.name == "Option" && receiver.arguments.size() == 1) {
                if (callee.value == "is_some" || callee.value == "is_none") {
                    if (supplied != 0) report(node, "Option." + callee.value + " expects no arguments");
                    return {TypeKind::Bool, {}};
                }
                if (callee.value == "unwrap") {
                    if (supplied != 0) report(node, "Option.unwrap expects no arguments");
                    return receiver.arguments[0];
                }
                if (callee.value == "unwrap_or") {
                    if (supplied != 1) report(node, "Option.unwrap_or expects 1 argument");
                    else {
                        const Type fallback = expression(*node.children[1]);
                        if (!is_assignable_type(receiver.arguments[0], fallback)) report(*node.children[1], "Option.unwrap_or fallback type mismatch");
                    }
                    return receiver.arguments[0];
                }
                report(callee, "Option has no method named '" + callee.value + "'");
                return {TypeKind::Error, {}};
            }
            if (receiver.kind == TypeKind::UserDefined && receiver.name == "Result" && receiver.arguments.size() == 2) {
                if (callee.value == "is_ok" || callee.value == "is_err") {
                    if (supplied != 0) report(node, "Result." + callee.value + " expects no arguments");
                    return {TypeKind::Bool, {}};
                }
                if (callee.value == "unwrap") { if (supplied != 0) report(node, "Result.unwrap expects no arguments"); return receiver.arguments[0]; }
                if (callee.value == "unwrap_err") { if (supplied != 0) report(node, "Result.unwrap_err expects no arguments"); return receiver.arguments[1]; }
                report(callee, "Result has no method named '" + callee.value + "'");
                return {TypeKind::Error, {}};
            }
            if (receiver.kind == TypeKind::UserDefined && receiver.name == "Map" && receiver.arguments.size() == 2) {
                if (callee.value == "insert") {
                    if (supplied != 2) report(node, "Map.insert expects 2 arguments");
                    else {
                        const Type key = expression(*node.children[1]); const Type value = expression(*node.children[2]);
                        if (!is_assignable_type(receiver.arguments[0], key)) report(*node.children[1], "Map key type mismatch");
                        if (!is_assignable_type(receiver.arguments[1], value)) report(*node.children[2], "Map value type mismatch");
                    }
                    return {TypeKind::Void, {}};
                }
                if (callee.value == "contains" || callee.value == "remove" || callee.value == "get") {
                    if (supplied != 1) report(node, "Map." + callee.value + " expects 1 argument");
                    else { const Type key = expression(*node.children[1]); if (!is_assignable_type(receiver.arguments[0], key)) report(*node.children[1], "Map key type mismatch"); }
                    if (callee.value == "get") return {TypeKind::UserDefined, "Option", {receiver.arguments[1]}, 0};
                    return {TypeKind::Bool, {}};
                }
                if (callee.value == "length") { if (supplied != 0) report(node, "Map.length expects no arguments"); return {TypeKind::U64, {}}; }
                if (callee.value == "clear") { if (supplied != 0) report(node, "Map.clear expects no arguments"); return {TypeKind::Void, {}}; }
                report(callee, "Map has no method named '" + callee.value + "'"); return {TypeKind::Error, {}};
            }
            if (receiver.kind == TypeKind::UserDefined && receiver.name == "Set" && receiver.arguments.size() == 1) {
                if (callee.value == "insert" || callee.value == "contains" || callee.value == "remove") {
                    if (supplied != 1) report(node, "Set." + callee.value + " expects 1 argument");
                    else { const Type value = expression(*node.children[1]); if (!is_assignable_type(receiver.arguments[0], value)) report(*node.children[1], "Set value type mismatch"); }
                    return callee.value == "insert" ? Type{TypeKind::Void, {}} : Type{TypeKind::Bool, {}};
                }
                if (callee.value == "length") { if (supplied != 0) report(node, "Set.length expects no arguments"); return {TypeKind::U64, {}}; }
                if (callee.value == "clear") { if (supplied != 0) report(node, "Set.clear expects no arguments"); return {TypeKind::Void, {}}; }
                report(callee, "Set has no method named '" + callee.value + "'"); return {TypeKind::Error, {}};
            }

            if (receiver.kind == TypeKind::Str) {
                const auto require = [&](std::size_t count) {
                    if (supplied != count) {
                        report(
                            node,
                            "str." + callee.value + " expects " +
                                std::to_string(count) + " arguments"
                        );
                        return false;
                    }
                    return true;
                };

                if (callee.value == "length") {
                    require(0);
                    return {TypeKind::U64, {}};
                }
                if (callee.value == "byte_at") {
                    if (require(1)) {
                        const Type argument = expression(*node.children[1]);
                        if (argument.kind != TypeKind::U64 && argument.kind != TypeKind::I64 && argument.kind != TypeKind::Error) {
                            report(*node.children[1], "str.byte_at expects an integer index");
                        }
                    }
                    return {TypeKind::I64, {}};
                }
                if (
                    callee.value == "starts_with" ||
                    callee.value == "ends_with" ||
                    callee.value == "contains"
                ) {
                    if (require(1)) {
                        const Type argument = expression(*node.children[1]);
                        if (argument.kind != TypeKind::Str && argument.kind != TypeKind::Error) {
                            report(*node.children[1], "str." + callee.value + " expects str");
                        }
                    }
                    return {TypeKind::Bool, {}};
                }
                if (callee.value == "find") {
                    if (require(1)) {
                        const Type argument = expression(*node.children[1]);
                        if (argument.kind != TypeKind::Str && argument.kind != TypeKind::Error) {
                            report(*node.children[1], "str.find expects str");
                        }
                    }
                    return {TypeKind::I64, {}};
                }
                if (callee.value == "slice") {
                    if (require(2)) {
                        for (std::size_t index = 1; index <= 2; ++index) {
                            const Type argument = expression(*node.children[index]);
                            if (argument.kind != TypeKind::U64 && argument.kind != TypeKind::I64 && argument.kind != TypeKind::Error) {
                                report(*node.children[index], "str.slice expects integer bounds");
                            }
                        }
                    }
                    return {TypeKind::Str, {}};
                }
                report(callee, "str has no method named '" + callee.value + "'");
                return {TypeKind::Error, {}};
            }

            if (receiver.kind != TypeKind::List) {
                report(callee, "method calls require a list or string value");
                return {TypeKind::Error, {}};
            }

            if (receiver.arguments.size() != 1) {
                return {TypeKind::Error, {}};
            }

            const Type element = receiver.arguments.front();
            const std::string list_name = type_name(receiver);
            if (callee.value == "push") {
                if (supplied != 1) {
                    report(
                        node,
                        list_name +
                            ".push expects exactly one argument"
                    );
                    return {TypeKind::Error, {}};
                }

                const Type value = expression(*node.children[1]);
                if (!is_assignable_type(element, value)) {
                    report(
                        *node.children[1],
                        list_name + ".push expects " +
                            type_name(element)
                    );
                }
                return {TypeKind::Void, {}};
            }

            if (callee.value == "pop") {
                if (supplied != 0) {
                    report(node, list_name + ".pop expects no arguments");
                }
                return element;
            }

            if (callee.value == "clear") {
                if (supplied != 0) {
                    report(node, list_name + ".clear expects no arguments");
                }
                return {TypeKind::Void, {}};
            }

            if (callee.value == "length" || callee.value == "capacity") {
                if (supplied != 0) {
                    report(
                        node,
                        list_name + "." + callee.value +
                            " expects no arguments"
                    );
                }
                return {TypeKind::U64, {}};
            }

            report(
                callee,
                list_name + " has no method named '" +
                    callee.value + "'"
            );
            return {TypeKind::Error, {}};
        }

        if (callee.kind != AstKind::IdentifierExpression) return {TypeKind::Error, {}};

        if (
            (callee.value.starts_with("List<") || callee.value.starts_with("Map<") || callee.value.starts_with("Set<")) &&
            callee.value.ends_with(">")
        ) {
            if (node.children.size() != 1) {
                report(
                    node,
                    callee.value +
                        " constructor expects no arguments"
                );
            }

            const Type list = parse_type_name(callee.value);
            const std::size_t expected_arguments = list.name == "Map" ? 2 : 1;
            if (list.arguments.size() != expected_arguments) return {TypeKind::Error, {}};

            const Type element = list.arguments.front();
            if (
                element.kind == TypeKind::Reference ||
                element.kind == TypeKind::Void ||
                element.kind == TypeKind::Unknown ||
                element.kind == TypeKind::Error
            ) {
                report(
                    callee,
                    "unsupported list element type: " +
                        type_name(element)
                );
                return {TypeKind::Error, {}};
            }

            return list;
        }

        if (structs_.contains(callee.value)) {
            return struct_construction(node, callee.value);
        }

        if (callee.value == "env_get" || callee.value == "process_run" || callee.value == "process_run_program") {
            if (node.children.size() != 2) report(node, callee.value + " expects 1 argument");
            else { const Type value = expression(*node.children[1]); if (value.kind != TypeKind::Str && value.kind != TypeKind::Error) report(*node.children[1], callee.value + " expects str"); }
            if (callee.value == "env_get") return {TypeKind::UserDefined, "Option", {Type{TypeKind::Str, {}}}, 0};
            return {TypeKind::I64, {}};
        }
        if (callee.value == "arg_count") {
            if (node.children.size() != 1) report(node, "arg_count expects no arguments");
            return {TypeKind::I64, {}};
        }
        if (callee.value == "arg") {
            if (node.children.size() != 2) report(node, "arg expects 1 argument");
            else {
                const Type value = expression(*node.children[1]);
                if (value.kind != TypeKind::I64 && value.kind != TypeKind::Error) report(*node.children[1], "arg expects i64");
            }
            return {TypeKind::Str, {}};
        }
        if (callee.value == "set_current_directory" || callee.value == "env_unset" || callee.value == "utf8_length" || callee.value == "byte_length") {
            if (node.children.size() != 2) report(node, callee.value + " expects 1 argument");
            else {
                const Type value = expression(*node.children[1]);
                if (value.kind != TypeKind::Str && value.kind != TypeKind::Error) report(*node.children[1], callee.value + " expects str");
            }
            if (callee.value == "utf8_length" || callee.value == "byte_length") return {TypeKind::I64, {}};
            return {TypeKind::Bool, {}};
        }
        if (callee.value == "env_set") {
            if (node.children.size() != 3) report(node, "env_set expects 2 arguments");
            else for (std::size_t index = 1; index <= 2; ++index) {
                const Type value = expression(*node.children[index]);
                if (value.kind != TypeKind::Str && value.kind != TypeKind::Error) report(*node.children[index], "env_set expects str arguments");
            }
            return {TypeKind::Bool, {}};
        }
        if (callee.value == "utf8_scalar_at") {
            if (node.children.size() != 3) report(node, "utf8_scalar_at expects 2 arguments");
            else {
                const Type text = expression(*node.children[1]);
                const Type index = expression(*node.children[2]);
                if (text.kind != TypeKind::Str && text.kind != TypeKind::Error) report(*node.children[1], "utf8_scalar_at expects str first");
                if (index.kind != TypeKind::I64 && index.kind != TypeKind::Error) report(*node.children[2], "utf8_scalar_at expects i64 index");
            }
            return {TypeKind::I64, {}};
        }
        if (callee.value == "time_unix_ms" || callee.value == "read_line") {
            if (node.children.size() != 1) report(node, callee.value + " expects no arguments");
            return callee.value == "read_line" ? Type{TypeKind::Str, {}} : Type{TypeKind::I64, {}};
        }
        if (callee.value == "sleep_ms" || callee.value == "i64_to_string") {
            if (node.children.size() != 2) report(node, callee.value + " expects 1 argument");
            else {
                const Type value = expression(*node.children[1]);
                if (value.kind != TypeKind::I64 && value.kind != TypeKind::Error) report(*node.children[1], callee.value + " expects i64");
            }
            return callee.value == "sleep_ms" ? Type{TypeKind::Void, {}} : Type{TypeKind::Str, {}};
        }
        if (callee.value == "try_parse_i64") {
            if (node.children.size() != 2) report(node, "try_parse_i64 expects 1 argument");
            else {
                const Type value = expression(*node.children[1]);
                if (value.kind != TypeKind::Str && value.kind != TypeKind::Error) report(*node.children[1], "try_parse_i64 expects str");
            }
            return {TypeKind::UserDefined, "Option", {Type{TypeKind::I64, {}}}, 0};
        }
        if (callee.value == "current_directory") {
            if (node.children.size() != 1) report(node, "current_directory expects no arguments");
            return {TypeKind::Str, {}};
        }

        if (
            callee.value == "fs_exists" || callee.value == "fs_read" ||
            callee.value == "path_is_absolute" || callee.value == "fs_create_directory" ||
            callee.value == "fs_create_directories" || callee.value == "fs_remove" ||
            callee.value == "fs_is_file" || callee.value == "fs_is_directory" ||
            callee.value == "path_parent" || callee.value == "path_filename" ||
            callee.value == "path_extension" || callee.value == "path_normalize" ||
            callee.value == "fs_remove_all" || callee.value == "fs_file_size"
        ) {
            if (node.children.size() != 2) report(node, callee.value + " expects 1 argument");
            else {
                const Type path = expression(*node.children[1]);
                if (path.kind != TypeKind::Str && path.kind != TypeKind::Error) report(*node.children[1], callee.value + " expects str");
            }
            const bool returns_string = callee.value == "fs_read" || callee.value == "path_parent" ||
                callee.value == "path_filename" || callee.value == "path_extension" || callee.value == "path_normalize";
            if (callee.value == "fs_remove_all" || callee.value == "fs_file_size") return {TypeKind::I64, {}};
            return returns_string ? Type{TypeKind::Str, {}} : Type{TypeKind::Bool, {}};
        }
        if (callee.value == "path_join" || callee.value == "fs_write" || callee.value == "fs_rename" || callee.value == "fs_copy" || callee.value == "process_run_arg") {
            if (node.children.size() != 3) report(node, callee.value + " expects 2 arguments");
            else for (std::size_t index = 1; index <= 2; ++index) {
                const Type value = expression(*node.children[index]);
                if (value.kind != TypeKind::Str && value.kind != TypeKind::Error) report(*node.children[index], callee.value + " expects str arguments");
            }
            return callee.value == "path_join" ? Type{TypeKind::Str, {}} : Type{TypeKind::Bool, {}};
        }

        if (callee.value == "print") {
            if (node.children.size() != 2) {
                report(node, "built-in 'print' expects exactly one argument");
            }
            for (std::size_t index = 1; index < node.children.size(); ++index) {
                const Type argument = expression(*node.children[index]);
                if (argument.kind == TypeKind::Void) {
                    report(*node.children[index], "cannot print a void value");
                }
            }
            return {TypeKind::Void, {}};
        }

        const auto function = functions_.find(callee.value);
        if (function == functions_.end()) return {TypeKind::Error, {}};
        const std::size_t supplied = node.children.size() - 1;
        if (supplied != function->second.parameters.size()) {
            report(node, "function '" + callee.value + "' expects " +
                std::to_string(function->second.parameters.size()) + " arguments but received " +
                std::to_string(supplied));
        }
        const std::size_t count = supplied < function->second.parameters.size()
            ? supplied : function->second.parameters.size();
        for (std::size_t index = 0; index < count; ++index) {
            const auto& argument = *node.children[index + 1];
            const Type received = expression(argument);
            const Type expected = function->second.parameters[index];
            if (
                expected.kind == TypeKind::List &&
                argument.kind != AstKind::MoveExpression &&
                argument.kind != AstKind::CallExpression
            ) {
                report(
                    argument,
                    "owned argument " +
                        std::to_string(index + 1) +
                        " of '" + callee.value +
                        "' requires an explicit move"
                );
            }
            if (!is_assignable_type(expected, received)) {
                report(*node.children[index + 1], "argument " + std::to_string(index + 1) +
                    " of '" + callee.value + "' expects " + std::string(type_name(expected)) +
                    " but received " + std::string(type_name(received)));
            }
        }
        return function->second.result;
    }

    static bool borrowable(const AstNode& node) {
        return node.kind == AstKind::IdentifierExpression ||
               node.kind == AstKind::MemberExpression ||
               node.kind == AstKind::IndexExpression;
    }

    Type borrow_expression(const AstNode& node) {
        if (node.children.empty()) return {TypeKind::Error, {}};
        const AstNode& operand = *node.children.front();
        const Type borrowed = expression(operand);
        if (!borrowable(operand)) {
            report(node, "cannot borrow a temporary value");
        }
        if (borrowed.kind == TypeKind::Reference) {
            report(node, "references to references are not supported");
            return {TypeKind::Error, {}};
        }
        return {TypeKind::Reference, {}, {borrowed}, 0};
    }

    Type move_expression(const AstNode& node) {
        if (
            node.children.size() != 1 ||
            node.children.front()->kind !=
                AstKind::IdentifierExpression
        ) {
            report(node, "move operand must be a variable");
            return {TypeKind::Error, {}};
        }

        const Type operand =
            expression(*node.children.front());

        if (
            operand.kind != TypeKind::List &&
            operand.kind != TypeKind::Error
        ) {
            report(
                node,
                "move requires an owned value, not " +
                    type_name(operand)
            );
            return {TypeKind::Error, {}};
        }

        return operand;
    }

    Type dereference_expression(const AstNode& node) {
        if (node.children.empty()) return {TypeKind::Error, {}};
        const Type operand = expression(*node.children.front());
        if (operand.kind != TypeKind::Reference || operand.arguments.size() != 1) {
            if (operand.kind != TypeKind::Error) {
                report(node, "dereference requires a reference value");
            }
            return {TypeKind::Error, {}};
        }
        return operand.arguments.front();
    }

    Type assignment_expression(const AstNode& node) {
        if (node.children.size() != 2) return {TypeKind::Error, {}};
        const Type target = expression(*node.children.front());
        const Type value = expression(*node.children.back());
        if (!is_assignable_type(target, value)) {
            report(node, "cannot assign " + std::string(type_name(value)) +
                " to variable of type " + std::string(type_name(target)));
        }
        if (node.value != "Equal" && !is_numeric_type(target)) {
            report(node, "compound assignment requires a numeric variable");
        }
        return target;
    }

    Type unary_expression(const AstNode& node) {
        if (node.children.empty()) return {TypeKind::Error, {}};
        const Type operand = expression(*node.children.front());
        if (node.value == "Bang") {
            if (operand.kind != TypeKind::Bool && operand.kind != TypeKind::Error) {
                report(node, "operator '!' requires a bool operand");
                return {TypeKind::Error, {}};
            }
            return {TypeKind::Bool, {}};
        }
        if (!is_numeric_type(operand) && operand.kind != TypeKind::Error) {
            report(node, "unary numeric operator requires a numeric operand");
            return {TypeKind::Error, {}};
        }
        return operand;
    }

    Type binary_expression(const AstNode& node) {
        if (node.children.size() != 2) return {TypeKind::Error, {}};
        const Type left = expression(*node.children.front());
        const Type right = expression(*node.children.back());

        if (node.value == "AmpAmp" || node.value == "PipePipe") {
            if (left.kind != TypeKind::Bool || right.kind != TypeKind::Bool) {
                report(node, "logical operator requires bool operands");
                return {TypeKind::Error, {}};
            }
            return {TypeKind::Bool, {}};
        }

        if (node.value == "EqualEqual" || node.value == "BangEqual") {
            if (!is_assignable_type(left, right)) {
                report(node, "equality operands must have matching types");
                return {TypeKind::Error, {}};
            }
            return {TypeKind::Bool, {}};
        }

        if (node.value == "Less" || node.value == "LessEqual" ||
            node.value == "Greater" || node.value == "GreaterEqual") {
            if (!is_numeric_type(left) || left != right) {
                report(node, "comparison requires matching numeric operands");
                return {TypeKind::Error, {}};
            }
            return {TypeKind::Bool, {}};
        }

        if (!is_numeric_type(left) || left != right) {
            report(node, "arithmetic operator requires matching numeric operands");
            return {TypeKind::Error, {}};
        }
        return left;
    }

    std::unordered_map<std::string, StructType> structs_;
    std::unordered_map<std::string, std::unordered_map<std::string, Type>> enums_;
    std::unordered_map<std::string, FunctionType> functions_;
    std::vector<std::unordered_map<std::string, Type>> scopes_;
    std::vector<Diagnostic> diagnostics_;
    std::string current_function_;
    Type current_return_{TypeKind::Void, {}};
};

}

bool TypeCheckResult::failed() const noexcept { return !diagnostics.empty(); }

TypeCheckResult check_types(const AstNode& root) {
    return TypeChecker().check(root);
}

}
