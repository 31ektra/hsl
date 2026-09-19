#pragma once

#include "hsl/ast.hpp"

#include <filesystem>
#include <string>

namespace hsl {

struct CodeGenerationResult {
    bool success;
    std::string message;
};

std::string generate_cpp(const AstNode& root);
CodeGenerationResult build_native(
    const AstNode& root,
    const std::filesystem::path& source_path,
    bool keep_generated = false
);
CodeGenerationResult emit_object(
    const AstNode& root,
    const std::filesystem::path& source_path
);
CodeGenerationResult emit_assembly(
    const AstNode& root,
    const std::filesystem::path& source_path
);

}
