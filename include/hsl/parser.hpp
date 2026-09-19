#pragma once

#include "hsl/ast.hpp"
#include "hsl/diagnostic.hpp"
#include "hsl/token.hpp"

#include <memory>
#include <vector>

namespace hsl {

struct ParseResult {
    std::unique_ptr<AstNode> root;
    std::vector<Diagnostic> diagnostics;

    bool failed() const noexcept;
};

ParseResult parse_tokens(const std::vector<Token>& tokens);

}
