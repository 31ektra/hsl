#include "hsl/ast.hpp"
#include "hsl/code_generator.hpp"
#include "hsl/compiler.hpp"
#include "hsl/lexer.hpp"
#include "hsl/module_loader.hpp"
#include "hsl/parser.hpp"
#include "hsl/semantic.hpp"
#include "hsl/type_checker.hpp"
#include "hsl/source.hpp"
#include "hsl/token.hpp"

#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hsl {
constexpr std::string_view VERSION = "9.0.0";



int show_tokens(const char* path) {
    try {
        const auto source = SourceFile::load(path);
        const auto result = lex_source(source.text());

        for (const auto& token : result.tokens) {
            std::cout << std::left
                      << std::setw(15)
                      << kind_name(token.kind)
                      << token.span.line
                      << ':'
                      << token.span.column;

            if (!token.text.empty() && token.kind != Kind::Newline) {
                std::cout << "  \"" << token.text << '"';
            }

            std::cout << '\n';
        }

        for (const auto& diagnostic : result.diagnostics) {
            std::cerr << diagnostic_severity_name(diagnostic.severity)
                      << ' '
                      << diagnostic.span.line
                      << ':'
                      << diagnostic.span.column
                      << ": "
                      << diagnostic.message
                      << '\n';
        }

        return result.failed() ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "hsl: " << error.what() << '\n';
        return 1;
    }
}
int show_ast(const char* path) {
    try {
        const auto source = SourceFile::load(path);
        const auto lex_result = lex_source(source.text());

        for (const auto& diagnostic : lex_result.diagnostics) {
            std::cerr << diagnostic_severity_name(diagnostic.severity)
                      << ' ' << diagnostic.span.line << ':' << diagnostic.span.column
                      << ": " << diagnostic.message << '\n';
        }

        if (lex_result.failed()) return 1;

        const auto parse_result = parse_tokens(lex_result.tokens);
        for (const auto& diagnostic : parse_result.diagnostics) {
            std::cerr << diagnostic_severity_name(diagnostic.severity)
                      << ' ' << diagnostic.span.line << ':' << diagnostic.span.column
                      << ": " << diagnostic.message << '\n';
        }

        if (parse_result.root) print_ast(*parse_result.root);
        return parse_result.failed() ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "hsl: " << error.what() << '\n';
        return 1;
    }
}

void print_diagnostics(
    const std::vector<Diagnostic>& diagnostics
) {
    for (const auto& diagnostic : diagnostics) {
        std::cerr
            << diagnostic_severity_name(diagnostic.severity)
            << ' '
            << diagnostic.span.line
            << ':'
            << diagnostic.span.column
            << ": "
            << diagnostic.message
            << '\n';
    }
}

int check_source(const char* path) {
    try {
        auto result = compile_source_tree(path);

        print_diagnostics(result.diagnostics);

        return result.failed() ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "hsl: " << error.what() << '\n';
        return 1;
    }
}
int emit_cpp_source(const char* path) {
    try {
        auto result = compile_source_tree(path);
        print_diagnostics(result.diagnostics);
        if (result.failed() || !result.root) return 1;
        std::cout << generate_cpp(*result.root);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hsl: " << error.what() << '\n';
        return 1;
    }
}

int verify_determinism(const char* path) {
    try {
        auto result = compile_source_tree(path);
        print_diagnostics(result.diagnostics);
        if (result.failed() || !result.root) return 1;
        const auto first = generate_cpp(*result.root);
        const auto second = generate_cpp(*result.root);
        if (first != second) {
            std::cerr << "hsl: nondeterministic generated output\n";
            return 1;
        }
        std::cout << "deterministic\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hsl: " << error.what() << '\n';
        return 1;
    }
}

int emit_native_artifact(const char* path, bool assembly) {
    try {
        auto result = compile_source_tree(path);
        print_diagnostics(result.diagnostics);
        if (result.failed() || !result.root) return 1;
        const auto generation = assembly
            ? emit_assembly(*result.root, path)
            : emit_object(*result.root, path);
        if (!generation.success) {
            std::cerr << "hsl: " << generation.message << '\n';
            return 1;
        }
        std::cout << generation.message << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hsl: " << error.what() << '\n';
        return 1;
    }
}

int build_source(const char* path, bool keep_generated = false) {
    try {
        auto result = compile_source_tree(path);

        print_diagnostics(result.diagnostics);

        if (result.failed() || !result.root) {
            return 1;
        }

        const auto generation =
            build_native(*result.root, path, keep_generated);

        if (!generation.success) {
            std::cerr
                << "hsl: "
                << generation.message
                << '\n';

            return 1;
        }

        std::cout << generation.message << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hsl: " << error.what() << '\n';
        return 1;
    }
}

}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--version") {
        std::cout << "HSL " << hsl::VERSION << '\n';
        return 0;
    }
    if (argc == 3 && std::string_view(argv[1]) == "tokens")
        return hsl::show_tokens(argv[2]);
    if (argc == 3 && std::string_view(argv[1]) == "ast")
        return hsl::show_ast(argv[2]);
    if (argc == 3 && std::string_view(argv[1]) == "check")
        return hsl::check_source(argv[2]);
    if (argc == 3 && std::string_view(argv[1]) == "emit-cpp")
        return hsl::emit_cpp_source(argv[2]);
    if (argc == 3 && std::string_view(argv[1]) == "emit-object")
        return hsl::emit_native_artifact(argv[2], false);
    if (argc == 3 && std::string_view(argv[1]) == "emit-assembly")
        return hsl::emit_native_artifact(argv[2], true);
    if (argc == 3 && std::string_view(argv[1]) == "verify-determinism")
        return hsl::verify_determinism(argv[2]);
    if (argc == 3 && std::string_view(argv[1]) == "build")
        return hsl::build_source(argv[2]);
    if (argc == 4 && std::string_view(argv[1]) == "build" && std::string_view(argv[2]) == "--keep-cpp")
        return hsl::build_source(argv[3], true);
    std::cout << "Usage:\n  hsl --version\n  hsl tokens <file>\n  hsl ast <file>\n  hsl check <file>\n  hsl emit-cpp <file>\n  hsl emit-object <file>\n  hsl emit-assembly <file>\n  hsl verify-determinism <file>\n  hsl build [--keep-cpp] <file>\n";
    return argc == 1 ? 0 : 1;
}
