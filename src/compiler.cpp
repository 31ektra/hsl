#include "hsl/compiler.hpp"

#include "hsl/module_loader.hpp"
#include "hsl/semantic.hpp"
#include "hsl/type_checker.hpp"

#include <iterator>
#include <utility>

namespace hsl {
namespace {

void append_diagnostics(
    std::vector<Diagnostic>& destination,
    std::vector<Diagnostic>& source
) {
    destination.insert(
        destination.end(),
        std::make_move_iterator(source.begin()),
        std::make_move_iterator(source.end())
    );
}

}

bool CompilationResult::failed() const noexcept {
    return !diagnostics.empty() || !root;
}

CompilationResult compile_source_tree(
    const std::filesystem::path& entry_path
) {
    auto module_result = load_module_tree(entry_path);

    CompilationResult result{
        std::move(module_result.root),
        {}
    };

    append_diagnostics(
        result.diagnostics,
        module_result.diagnostics
    );

    if (!result.root || !result.diagnostics.empty()) {
        return result;
    }

    auto semantic_result = analyze_semantics(*result.root);

    append_diagnostics(
        result.diagnostics,
        semantic_result.diagnostics
    );

    if (!result.diagnostics.empty()) {
        return result;
    }

    auto type_result = check_types(*result.root);

    append_diagnostics(
        result.diagnostics,
        type_result.diagnostics
    );

    return result;
}

}
