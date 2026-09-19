#include "hsl/semantic.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace hsl {
namespace {

struct VariableInfo {
    bool mutable_value;
    bool moved = false;
};

class SemanticAnalyzer {
public:
    SemanticResult analyze(const AstNode& root) {
        collect_structs(root);
        collect_enums(root);
        collect_functions(root);
        for (const auto& child : root.children) {
            if (child->kind == AstKind::Function) analyze_function(*child);
        }
        if (!functions_.contains("main")) {
            report(root, "program must declare a 'main' function");
        }
        return {std::move(diagnostics_)};
    }

private:
    void report(const AstNode& node, std::string message) {
        diagnostics_.push_back({DiagnosticSeverity::Error, node.span, std::move(message)});
    }

    static std::string_view name_part(std::string_view label) {
        const auto separator = label.find(':');
        return label.substr(0, separator);
    }

    void collect_enums(const AstNode& root) {
        for (const auto& child : root.children) {
            if (child->kind != AstKind::EnumDeclaration) continue;
            if (!enums_.insert(child->value).second) {
                report(*child, "duplicate enum declaration: " + child->value);
            }
            std::unordered_set<std::string> variants;
            for (const auto& variant : child->children) {
                const std::string variant_name(name_part(variant->value));
                if (!variants.insert(variant_name).second) {
                    report(*variant, "duplicate enum variant: " + variant_name);
                }
            }
        }
    }

    void collect_structs(const AstNode& root) {
        for (const auto& child : root.children) {
            if (child->kind != AstKind::StructDeclaration) {
                continue;
            }

            const std::string struct_name = child->value;

            if (!structs_.insert(struct_name).second) {
                report(
                    *child,
                    "duplicate struct declaration: " + struct_name
                );
            }

            std::unordered_set<std::string> fields;

            for (const auto& field : child->children) {
                if (field->kind != AstKind::FieldDeclaration) {
                    continue;
                }

                const std::string field_name(name_part(field->value));

                if (!fields.insert(field_name).second) {
                    report(
                        *field,
                        "duplicate field declaration: " + field_name
                    );
                }
            }
        }
    }

    void collect_functions(const AstNode& root) {
        for (const auto& child : root.children) {
            if (child->kind != AstKind::Function) continue;
            const std::string name(name_part(child->value));
            if (!functions_.insert(name).second) {
                report(*child, "duplicate function declaration: " + name);
            }
        }
    }

    void analyze_function(const AstNode& function) {
        scopes_.clear();
        push_scope();

        for (const auto& child : function.children) {
            if (child->kind != AstKind::Parameter) continue;
            declare(*child, true, "duplicate parameter declaration: ");
        }

        for (const auto& child : function.children) {
            if (child->kind == AstKind::Block) analyze_block(*child, false);
        }

        pop_scope();
    }

    void push_scope() { scopes_.emplace_back(); }
    void pop_scope() { scopes_.pop_back(); }

    void declare(const AstNode& node, bool mutable_value, std::string_view prefix) {
        const std::string name(name_part(node.value));
        auto& scope = scopes_.back();
        if (scope.contains(name)) {
            report(node, std::string(prefix) + name);
            return;
        }
        scope.emplace(name, VariableInfo{mutable_value, false});
    }

    VariableInfo* find_variable(std::string_view name) {
        for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
            const auto item = scope->find(std::string(name));
            if (item != scope->end()) return &item->second;
        }
        return nullptr;
    }

    const VariableInfo* find_variable(std::string_view name) const {
        for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
            const auto item = scope->find(std::string(name));
            if (item != scope->end()) return &item->second;
        }
        return nullptr;
    }

    void analyze_block(const AstNode& block, bool create_scope = true) {
        if (create_scope) push_scope();
        for (const auto& statement : block.children) analyze_statement(*statement);
        if (create_scope) pop_scope();
    }

    void analyze_statement(const AstNode& node) {
        switch (node.kind) {
            case AstKind::LetDeclaration:
                if (!node.children.empty()) analyze_expression(*node.children.front());
                declare(node, false, "duplicate local declaration: ");
                return;
            case AstKind::VarDeclaration:
                if (!node.children.empty()) analyze_expression(*node.children.front());
                declare(node, true, "duplicate local declaration: ");
                return;
            case AstKind::MatchStatement: {
                if (node.children.empty()) return;
                analyze_expression(*node.children.front());
                const auto before = scopes_;
                std::vector<decltype(scopes_)> arm_states;
                for (std::size_t index = 1; index < node.children.size(); ++index) {
                    scopes_ = before;
                    const auto& arm = *node.children[index];
                    push_scope();
                    const auto separator = arm.value.find(':');
                    if (separator != std::string::npos) scopes_.back()[arm.value.substr(separator + 1)] = {false, false};
                    if (!arm.children.empty()) analyze_block(*arm.children.front(), false);
                    pop_scope();
                    arm_states.push_back(scopes_);
                }
                scopes_ = before;
                if (!arm_states.empty()) {
                    for (std::size_t scope_index = 0; scope_index < scopes_.size(); ++scope_index) {
                        for (auto& [name, variable] : scopes_[scope_index]) {
                            bool moved_in_all = true;
                            for (const auto& state : arm_states) {
                                const auto found = state[scope_index].find(name);
                                moved_in_all = moved_in_all && found != state[scope_index].end() && found->second.moved;
                            }
                            variable.moved = moved_in_all;
                        }
                    }
                }
                return;
            }
            case AstKind::ForStatement:
                if (!node.children.empty()) analyze_expression(*node.children.front());
                push_scope();
                declare(node, false, "duplicate loop variable: ");
                if (node.children.size() > 1) analyze_block(*node.children[1], false);
                pop_scope();
                return;
            case AstKind::ReturnStatement:
            case AstKind::ExpressionStatement:
                for (const auto& child : node.children) analyze_expression(*child);
                return;
            case AstKind::IfStatement: {
                if (node.children.empty()) return;
                analyze_expression(*node.children.front());
                const auto before = scopes_;
                auto then_state = before;
                auto else_state = before;
                bool has_then = false;
                bool has_else = false;
                for (std::size_t index = 1; index < node.children.size(); ++index) {
                    const auto& child = *node.children[index];
                    if (child.kind == AstKind::Block) {
                        scopes_ = before;
                        analyze_block(child);
                        then_state = scopes_;
                        has_then = true;
                    } else if (child.kind == AstKind::ElseClause && !child.children.empty()) {
                        scopes_ = before;
                        analyze_block(*child.children.front());
                        else_state = scopes_;
                        has_else = true;
                    }
                }
                scopes_ = before;
                if (has_then && has_else) {
                    for (std::size_t scope_index = 0; scope_index < scopes_.size(); ++scope_index) {
                        for (auto& [name, variable] : scopes_[scope_index]) {
                            const auto then_var = then_state[scope_index].find(name);
                            const auto else_var = else_state[scope_index].find(name);
                            if (then_var != then_state[scope_index].end() && else_var != else_state[scope_index].end()) {
                                variable.moved = then_var->second.moved && else_var->second.moved;
                            }
                        }
                    }
                }
                return;
            }
            case AstKind::WhileStatement:
                if (!node.children.empty()) analyze_expression(*node.children.front());
                for (std::size_t index = 1; index < node.children.size(); ++index) {
                    const auto& child = *node.children[index];
                    if (child.kind == AstKind::Block) analyze_block(child);
                }
                return;
            default:
                analyze_expression(node);
                return;
        }
    }

    const AstNode* assignment_root(const AstNode& node) const {
        const AstNode* current = &node;

        while (
            (current->kind == AstKind::MemberExpression ||
             current->kind == AstKind::IndexExpression) &&
            !current->children.empty()
        ) {
            current = current->children.front().get();
        }

        if (current->kind != AstKind::IdentifierExpression) {
            return nullptr;
        }

        return current;
    }

    void analyze_assignment_target(const AstNode& target) {
        if (
            target.kind != AstKind::IdentifierExpression &&
            target.kind != AstKind::MemberExpression &&
            target.kind != AstKind::IndexExpression &&
            target.kind != AstKind::DereferenceExpression
        ) {
            report(target, "assignment target must be a variable or field");
            analyze_expression(target);
            return;
        }

        if (target.kind == AstKind::DereferenceExpression) {
            report(target, "cannot assign through an immutable reference");
            analyze_expression(target);
            return;
        }

        const AstNode* root = assignment_root(target);

        if (!root) {
            report(target, "assignment target must be rooted in a variable");
            analyze_expression(target);
            return;
        }

        const auto* variable = find_variable(root->value);

        if (!variable) {
            report(*root, "undefined identifier: " + root->value);
            return;
        }

        if (!variable->mutable_value) {
            if (target.kind != AstKind::IdentifierExpression) {
                report(
                    *root,
                    "cannot assign through immutable variable: " +
                        root->value
                );
            } else {
                report(
                    *root,
                    "cannot assign to immutable variable: " +
                        root->value
                );
            }
        }

        if (target.kind != AstKind::IdentifierExpression) {
            analyze_expression(target);
        }
    }

    void analyze_expression(const AstNode& node) {
        switch (node.kind) {
            case AstKind::IdentifierExpression: {
                const auto* variable = find_variable(node.value);
                if (!variable) {
                    report(node, "undefined identifier: " + node.value);
                } else if (variable->moved) {
                    report(node, "use of moved value: " + node.value);
                }
                return;
            }
            case AstKind::MoveExpression: {
                if (
                    node.children.size() != 1 ||
                    node.children.front()->kind !=
                        AstKind::IdentifierExpression
                ) {
                    report(
                        node,
                        "move operand must be a variable"
                    );
                    for (const auto& child : node.children) {
                        analyze_expression(*child);
                    }
                    return;
                }

                const auto& operand = *node.children.front();
                auto* variable = find_variable(operand.value);
                if (!variable) {
                    report(
                        operand,
                        "undefined identifier: " + operand.value
                    );
                    return;
                }
                if (variable->moved) {
                    report(
                        operand,
                        "use of moved value: " + operand.value
                    );
                    return;
                }
                variable->moved = true;
                return;
            }
            case AstKind::CallExpression:
                if (node.children.empty()) return;
                if (node.children.front()->kind == AstKind::IdentifierExpression) {
                    const std::string& function_name = node.children.front()->value;
                    const bool system_builtin =
                        function_name == "fs_exists" || function_name == "fs_read" ||
                        function_name == "fs_write" || function_name == "path_join" ||
                        function_name == "path_is_absolute" || function_name == "env_get" ||
                        function_name == "process_run" || function_name == "process_run_program" || function_name == "current_directory" ||
                        function_name == "fs_create_directory" || function_name == "fs_create_directories" ||
                        function_name == "fs_remove" || function_name == "fs_rename" ||
                        function_name == "fs_is_file" || function_name == "fs_is_directory" ||
                        function_name == "path_parent" || function_name == "path_filename" ||
                        function_name == "path_extension" || function_name == "path_normalize" ||
                        function_name == "process_run_arg" || function_name == "fs_copy" ||
                        function_name == "fs_remove_all" || function_name == "fs_file_size" ||
                        function_name == "time_unix_ms" || function_name == "sleep_ms" ||
                        function_name == "read_line" || function_name == "i64_to_string" ||
                        function_name == "try_parse_i64" || function_name == "arg_count" ||
                        function_name == "arg" || function_name == "set_current_directory" ||
                        function_name == "env_set" || function_name == "env_unset" ||
                        function_name == "utf8_length" || function_name == "utf8_scalar_at" ||
                        function_name == "byte_length";
                    if (function_name != "print" && !system_builtin &&
                        !function_name.starts_with("List<") &&
                        !function_name.starts_with("Map<") &&
                        !function_name.starts_with("Set<") &&
                        !functions_.contains(function_name) &&
                        !structs_.contains(function_name)) {
                        report(
                            *node.children.front(),
                            "undefined function: " + function_name
                        );
                    }
                } else {
                    analyze_expression(*node.children.front());
                }
                for (std::size_t index = 1; index < node.children.size(); ++index) {
                    analyze_expression(*node.children[index]);
                }
                return;
            case AstKind::MemberExpression:
                if (!node.children.empty()) {
                    const auto& base = *node.children.front();
                    if (base.kind == AstKind::IdentifierExpression) {
                        if (enums_.contains(base.value)) return;
                        if (
                            base.value.starts_with("Option<") ||
                            base.value.starts_with("Result<") ||
                            base.value.starts_with("Map<") ||
                            base.value.starts_with("Set<")
                        ) return;
                    }
                    analyze_expression(base);
                }
                return;
            case AstKind::AssignmentExpression:
                if (node.children.size() != 2) {
                    return;
                }

                analyze_assignment_target(*node.children.front());
                analyze_expression(*node.children.back());
                return;
            case AstKind::IntegerLiteral:
            case AstKind::FloatLiteral:
            case AstKind::StringLiteral:
            case AstKind::BooleanLiteral:
                return;
            default:
                for (const auto& child : node.children) analyze_expression(*child);
                return;
        }
    }

    std::unordered_set<std::string> structs_;
    std::unordered_set<std::string> enums_;
    std::unordered_set<std::string> functions_;
    std::vector<std::unordered_map<std::string, VariableInfo>> scopes_;
    std::vector<Diagnostic> diagnostics_;
};

}

bool SemanticResult::failed() const noexcept { return !diagnostics.empty(); }

SemanticResult analyze_semantics(const AstNode& root) {
    return SemanticAnalyzer().analyze(root);
}

}
