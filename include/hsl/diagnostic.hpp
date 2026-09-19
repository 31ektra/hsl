#pragma once

#include "hsl/source_span.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace hsl {

enum class DiagnosticSeverity : std::uint8_t {
    Error,
    Warning,
    Note,
};

struct Diagnostic {
    DiagnosticSeverity severity;
    SourceSpan span;
    std::string message;
};

std::string_view diagnostic_severity_name(DiagnosticSeverity severity);

}
