#pragma once

#include "scioh/Token.hpp"

#include <string>
#include <string_view>

namespace scioh {

class Lexer {
public:
    explicit Lexer(std::string_view source);

    Token nextToken();

private:
    Token nextTokenInner();
    bool isAtEnd() const;
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    void skipTrivia();

    Token makeToken(TokenKind kind, std::string lexeme, SourceLocation location) const;
    Token identifier(SourceLocation start);
    Token number(SourceLocation start);
    Token string(SourceLocation start);

    std::string_view source_;
    std::size_t current_ = 0;
    SourceLocation location_;
};

} // namespace scioh
