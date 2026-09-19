#pragma once

#include "hsl/diagnostic.hpp"
#include "hsl/token.hpp"

#include <string_view>
#include <vector>

namespace hsl {

struct LexResult {
    std::vector<Token> tokens;
    std::vector<Diagnostic> diagnostics;

    bool failed() const noexcept;
};

LexResult lex_source(std::string_view source);

}
