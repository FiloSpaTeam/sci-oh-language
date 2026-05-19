#pragma once

#include "scioh/SourceLocation.hpp"

#include <string>

namespace scioh {

enum class TokenKind {
    EndOfFile,
    Identifier,
    Number,
    String,
    Let,
    Print,
    If,
    Else,
    End,
    True,
    False,
    Plus,
    Minus,
    Star,
    Slash,
    Equal,
    EqualEqual,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    De,
    And,
    Or,
    Not,
    AssignWord,
    Semicolon,
    LeftParen,
    RightParen,
    Comma,
    Function,
    Return,
    Lambda,
    Fine,
    Po,
    LeftBracket,
    RightBracket,
    Prime,
    Uldeme,
    Vute,
    Mitta,
    Simele,
    Cusci,
    Pipe,
    Damme,
    Numere,
    Quante,
    Percent,
    Spezza,
    Iecch,
    Cala,
    Suva,
    Arretunne,
    Radice,
    Quadrata,
    Vabbone,
    Guaje,
    Prove,
    Dove,
};

struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    std::string lexeme;
    SourceLocation location;
    bool followsWhitespace = false; // true when preceded by whitespace/comment
};

const char* tokenKindName(TokenKind kind);

} // namespace scioh
