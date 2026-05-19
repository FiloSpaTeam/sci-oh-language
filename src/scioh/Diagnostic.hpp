#pragma once

#include "scioh/SourceLocation.hpp"

#include <stdexcept>
#include <string>

namespace scioh {

class DiagnosticError : public std::runtime_error {
public:
    DiagnosticError(SourceLocation location, const std::string& message)
        : std::runtime_error(format(location, message)), location_(location) {}

    SourceLocation location() const { return location_; }

private:
    static std::string format(SourceLocation location, const std::string& message) {
        return std::to_string(location.line) + ":" + std::to_string(location.column) + ": " + message;
    }

    SourceLocation location_;
};

} // namespace scioh
