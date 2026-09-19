#pragma once

#include "hsl/ast.hpp"
#include "hsl/diagnostic.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace hsl {

struct ModuleLoadResult {
    std::unique_ptr<AstNode> root;
    std::vector<Diagnostic> diagnostics;

    bool failed() const noexcept;
};

ModuleLoadResult load_module_tree(
    const std::filesystem::path& entry_path
);

}
