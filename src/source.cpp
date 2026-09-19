#include "hsl/source.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace hsl {

SourceFile SourceFile::load(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);

    if (!input) {
        throw std::runtime_error(
            "cannot open source file: " + path.string()
        );
    }

    SourceFile source;
    source.path_ = path;
    source.text_ = std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );

    return source;
}

const std::filesystem::path& SourceFile::path() const noexcept {
    return path_;
}

const std::string& SourceFile::text() const noexcept {
    return text_;
}

}
