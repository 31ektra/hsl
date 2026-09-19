#pragma once

#include "hsl/ast.hpp"
#include "hsl/diagnostic.hpp"

#include <vector>

namespace hsl {

struct TypeCheckResult {
    std::vector<Diagnostic> diagnostics;

    bool failed() const noexcept;
};

TypeCheckResult check_types(const AstNode& root);

}
