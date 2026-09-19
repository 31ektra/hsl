#include "hsl/module_loader.hpp"

#include "hsl/lexer.hpp"
#include "hsl/parser.hpp"
#include "hsl/source.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace hsl {
namespace {

std::string_view name_part(std::string_view label) {
    const auto separator = label.find(':');
    return label.substr(0, separator);
}

std::string_view type_part(std::string_view label) {
    const auto separator = label.find(':');

    if (separator == std::string_view::npos) {
        return {};
    }

    return label.substr(separator + 1);
}

std::string make_label(
    std::string_view name,
    std::string_view type
) {
    std::string result(name);

    if (!type.empty()) {
        result += ':';
        result += type;
    }

    return result;
}

bool valid_module_name(std::string_view name) {
    if (name.empty()) {
        return false;
    }

    if (
        name.front() < 'a' ||
        name.front() > 'z' ||
        name.back() == '_'
    ) {
        return false;
    }

    bool previous_underscore = false;

    for (const char character : name) {
        const bool lowercase =
            character >= 'a' &&
            character <= 'z';

        const bool digit =
            character >= '0' &&
            character <= '9';

        if (lowercase || digit) {
            previous_underscore = false;
            continue;
        }

        if (character == '_' && !previous_underscore) {
            previous_underscore = true;
            continue;
        }

        return false;
    }

    return !previous_underscore;
}

std::filesystem::path normalized_path(
    const std::filesystem::path& path
) {
    std::error_code error;

    auto absolute = std::filesystem::absolute(path, error);

    if (error) {
        return path.lexically_normal();
    }

    auto canonical =
        std::filesystem::weakly_canonical(absolute, error);

    if (error) {
        return absolute.lexically_normal();
    }

    return canonical;
}

class ModuleLoader {
public:
    ModuleLoadResult load(
        const std::filesystem::path& entry_path
    ) {
        const auto normalized = normalized_path(entry_path);

        merged_ = std::make_unique<AstNode>(AstNode{
            AstKind::Program,
            SourceSpan{},
            {},
            {}
        });

        load_file(normalized, {}, true);

        return {
            std::move(merged_),
            std::move(diagnostics_)
        };
    }

private:
    void report(
        const AstNode& node,
        std::string message
    ) {
        diagnostics_.push_back({
            DiagnosticSeverity::Error,
            node.span,
            std::move(message)
        });
    }

    void report(
        SourceSpan span,
        std::string message
    ) {
        diagnostics_.push_back({
            DiagnosticSeverity::Error,
            span,
            std::move(message)
        });
    }

    std::string qualify(
        std::string_view module,
        std::string_view name
    ) const {
        if (module.empty()) {
            return std::string(name);
        }

        return std::string(module) +
            "__" +
            std::string(name);
    }

    std::string rewrite_type(
        std::string_view type,
        std::string_view module,
        const std::unordered_set<std::string>& local_structs,
        const std::unordered_map<std::string, std::string>& imports,
        const AstNode& owner
    ) {
        if (type.empty()) {
            return {};
        }

        if (type.front() == '&') {
            return "&" + rewrite_type(
                type.substr(1),
                module,
                local_structs,
                imports,
                owner
            );
        }

        if (
            type.starts_with("List<") &&
            type.ends_with(">")
        ) {
            const std::string element = rewrite_type(
                type.substr(5, type.size() - 6),
                module,
                local_structs,
                imports,
                owner
            );

            return "List<" + element + ">";
        }

        if (type.front() == '[' && type.back() == ']') {
            std::size_t depth = 0;
            std::size_t separator = std::string_view::npos;

            for (std::size_t index = 1; index + 1 < type.size(); ++index) {
                if (type[index] == '[') {
                    ++depth;
                } else if (type[index] == ']') {
                    --depth;
                } else if (type[index] == ';' && depth == 0) {
                    separator = index;
                    break;
                }
            }

            if (separator == std::string_view::npos) {
                const std::string element = rewrite_type(
                    type.substr(1, type.size() - 2),
                    module,
                    local_structs,
                    imports,
                    owner
                );

                return "[" + element + "]";
            }

            const std::string element = rewrite_type(
                type.substr(1, separator - 1),
                module,
                local_structs,
                imports,
                owner
            );

            return "[" + element +
                std::string(type.substr(separator));
        }

        const auto separator = type.find('.');

        if (separator != std::string_view::npos) {
            const std::string prefix(type.substr(0, separator));
            const std::string type_name(type.substr(separator + 1));

            const auto imported = imports.find(prefix);

            if (imported == imports.end()) {
                report(
                    owner,
                    "unknown imported module in type: " + prefix
                );

                return std::string(type);
            }

            if (type_name.empty()) {
                report(
                    owner,
                    "expected type name after '.'"
                );

                return std::string(type);
            }

            const auto exported =
                module_structs_.find(imported->second);

            if (
                exported == module_structs_.end() ||
                !exported->second.contains(type_name)
            ) {
                report(
                    owner,
                    "module '" + prefix +
                        "' has no type named '" +
                        type_name + "'"
                );

                return qualify(imported->second, type_name);
            }

            return qualify(imported->second, type_name);
        }

        if (local_structs.contains(std::string(type))) {
            return qualify(module, type);
        }

        return std::string(type);
    }

    void rewrite_label_type(
        AstNode& node,
        std::string_view module,
        const std::unordered_set<std::string>& local_structs,
        const std::unordered_map<std::string, std::string>& imports
    ) {
        const auto type = type_part(node.value);

        if (type.empty()) {
            return;
        }

        node.value = make_label(
            name_part(node.value),
            rewrite_type(
                type,
                module,
                local_structs,
                imports,
                node
            )
        );
    }

    bool collapse_qualified_reference(
        AstNode& node,
        const std::unordered_map<std::string, std::string>& imports
    ) {
        if (
            node.kind != AstKind::MemberExpression ||
            node.children.empty()
        ) {
            return false;
        }

        AstNode& base = *node.children.front();

        if (base.kind != AstKind::IdentifierExpression) {
            return false;
        }

        const auto imported = imports.find(base.value);

        if (imported == imports.end()) {
            return false;
        }

        node.kind = AstKind::IdentifierExpression;
        node.value = qualify(imported->second, node.value);
        node.children.clear();
        return true;
    }

    void rewrite_expression(
        AstNode& node,
        std::string_view module,
        const std::unordered_set<std::string>& local_functions,
        const std::unordered_set<std::string>& local_structs,
        const std::unordered_map<std::string, std::string>& imports
    ) {
        if (
            node.kind == AstKind::LetDeclaration ||
            node.kind == AstKind::VarDeclaration
        ) {
            rewrite_label_type(
                node,
                module,
                local_structs,
                imports
            );
        }

        if (collapse_qualified_reference(node, imports)) {
            return;
        }

        if (
            node.kind == AstKind::CallExpression &&
            !node.children.empty()
        ) {
            AstNode& callee = *node.children.front();

            collapse_qualified_reference(callee, imports);

            if (callee.kind == AstKind::IdentifierExpression) {
                if (
                    local_functions.contains(callee.value) ||
                    local_structs.contains(callee.value)
                ) {
                    callee.value = qualify(module, callee.value);
                }
            }
        }

        for (auto& child : node.children) {
            rewrite_expression(
                *child,
                module,
                local_functions,
                local_structs,
                imports
            );
        }
    }

    void rewrite_declaration(
        AstNode& declaration,
        std::string_view module,
        const std::unordered_set<std::string>& local_functions,
        const std::unordered_set<std::string>& local_structs,
        const std::unordered_map<std::string, std::string>& imports
    ) {
        if (declaration.kind == AstKind::StructDeclaration) {
            declaration.value = qualify(
                module,
                declaration.value
            );

            for (auto& field : declaration.children) {
                rewrite_label_type(
                    *field,
                    module,
                    local_structs,
                    imports
                );
            }

            return;
        }

        if (declaration.kind != AstKind::Function) {
            return;
        }

        const std::string function_name(
            name_part(declaration.value)
        );

        declaration.value = make_label(
            qualify(module, function_name),
            rewrite_type(
                type_part(declaration.value),
                module,
                local_structs,
                imports,
                declaration
            )
        );

        for (auto& child : declaration.children) {
            if (child->kind == AstKind::Parameter) {
                rewrite_label_type(
                    *child,
                    module,
                    local_structs,
                    imports
                );
            }

            rewrite_expression(
                *child,
                module,
                local_functions,
                local_structs,
                imports
            );
        }
    }

    std::string cycle_message(
        const std::filesystem::path& repeated
    ) const {
        std::string message = "import cycle detected: ";

        auto first = std::find(
            active_.begin(),
            active_.end(),
            repeated
        );

        bool separator = false;

        for (auto current = first; current != active_.end(); ++current) {
            if (separator) {
                message += " -> ";
            }

            message += current->stem().string();
            separator = true;
        }

        if (separator) {
            message += " -> ";
        }

        message += repeated.stem().string();
        return message;
    }

    void load_file(
        const std::filesystem::path& path,
        std::string module,
        bool entry
    ) {
        const auto normalized = normalized_path(path);
        const std::string key = normalized.string();

        if (loaded_.contains(key)) {
            return;
        }

        const auto active = std::find(
            active_.begin(),
            active_.end(),
            normalized
        );

        if (active != active_.end()) {
            report(
                SourceSpan{},
                cycle_message(normalized)
            );
            return;
        }

        if (!std::filesystem::is_regular_file(normalized)) {
            report(
                SourceSpan{},
                "cannot open source file: " +
                    normalized.string()
            );
            return;
        }

        active_.push_back(normalized);

        SourceFile source;

        try {
            source = SourceFile::load(normalized);
        } catch (const std::exception& error) {
            report(SourceSpan{}, error.what());
            active_.pop_back();
            return;
        }

        const auto lex_result = lex_source(source.text());

        diagnostics_.insert(
            diagnostics_.end(),
            lex_result.diagnostics.begin(),
            lex_result.diagnostics.end()
        );

        if (lex_result.failed()) {
            active_.pop_back();
            return;
        }

        auto parse_result = parse_tokens(lex_result.tokens);

        diagnostics_.insert(
            diagnostics_.end(),
            parse_result.diagnostics.begin(),
            parse_result.diagnostics.end()
        );

        if (parse_result.failed() || !parse_result.root) {
            active_.pop_back();
            return;
        }

        std::unordered_map<std::string, std::string> imports;

        for (const auto& child : parse_result.root->children) {
            if (child->kind != AstKind::ImportDeclaration) {
                continue;
            }

            if (!valid_module_name(child->value)) {
                report(
                    *child,
                    "module names must use lowercase snake_case"
                );
                continue;
            }

            imports[child->value] = child->value;

            auto imported_path = normalized.parent_path();
            imported_path /= child->value + ".hsl";

            if (!std::filesystem::is_regular_file(imported_path)) {
                report(
                    *child,
                    "cannot find imported module: " +
                        child->value
                );
                continue;
            }

            load_file(
                imported_path,
                child->value,
                false
            );
        }

        std::unordered_set<std::string> local_functions;
        std::unordered_set<std::string> local_structs;

        for (const auto& child : parse_result.root->children) {
            if (child->kind == AstKind::Function) {
                local_functions.insert(
                    std::string(name_part(child->value))
                );
            } else if (
                child->kind == AstKind::StructDeclaration
            ) {
                local_structs.insert(child->value);
            }
        }

        if (!module.empty()) {
            module_structs_[module] = local_structs;
        }

        for (auto& child : parse_result.root->children) {
            if (child->kind == AstKind::ImportDeclaration) {
                continue;
            }

            rewrite_declaration(
                *child,
                entry ? std::string_view{} : std::string_view(module),
                local_functions,
                local_structs,
                imports
            );

            merged_->children.push_back(std::move(child));
        }

        active_.pop_back();
        loaded_.insert(key);
    }

    std::unique_ptr<AstNode> merged_;
    std::vector<Diagnostic> diagnostics_;
    std::unordered_set<std::string> loaded_;
    std::unordered_map<
        std::string,
        std::unordered_set<std::string>
    > module_structs_;
    std::vector<std::filesystem::path> active_;
};

}

bool ModuleLoadResult::failed() const noexcept {
    return !diagnostics.empty();
}

ModuleLoadResult load_module_tree(
    const std::filesystem::path& entry_path
) {
    return ModuleLoader().load(entry_path);
}

}
