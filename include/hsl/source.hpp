#pragma once

#include <filesystem>
#include <string>

namespace hsl {

class SourceFile {
public:
    static SourceFile load(const std::filesystem::path& path);

    const std::filesystem::path& path() const noexcept;
    const std::string& text() const noexcept;

private:
    std::filesystem::path path_;
    std::string text_;
};

}
