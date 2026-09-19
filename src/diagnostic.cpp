#include "hsl/diagnostic.hpp"

namespace hsl {

std::string_view diagnostic_severity_name(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Error:
            return "error";
        case DiagnosticSeverity::Warning:
            return "warning";
        case DiagnosticSeverity::Note:
            return "note";
    }

    return "diagnostic";
}

}
