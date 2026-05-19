#pragma once

#include <cstddef>

namespace scioh {

struct SourceLocation {
    std::size_t line = 1;
    std::size_t column = 1;
};

} // namespace scioh
