#pragma once

#include <cstdint>

namespace hsl {

struct SourceSpan {
    std::uint32_t offset;
    std::uint32_t length;
    std::uint32_t line;
    std::uint32_t column;
};

}
