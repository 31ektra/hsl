#pragma once

#include "hsl/ast.hpp"
#include "hsl/diagnostic.hpp"

#include <vector>

namespace hsl {

struct SemanticResult {
    std::vector<Diagnostic> diagnostics;

    bool failed() const noexcept;
};

SemanticResult analyze_semantics(const AstNode& root);

}
