#include "scioh/Lexer.hpp"

#include "scioh/Diagnostic.hpp"

#include <cctype>
#include <string_view>
#include <utility>

namespace scioh {
namespace {

bool isIdentifierStart(char value) {
    const auto byte = static_cast<unsigned char>(value);
    return std::isalpha(byte) || value == '_' || byte >= 128;
}

bool isIdentifierPart(char value) {
    const auto byte = static_cast<unsigned char>(value);
    return std::isalnum(byte) || value == '_' || byte >= 128;
}

TokenKind keywordKind(std::string_view text) {
    if (text == "mitte" || text == "metti" || text == "remette" || text == "variabile") {
        return TokenKind::Let;
    }
    if (text == "dicce" || text == "di" || text == "scrive" || text == "stampa") {
        return TokenKind::Print;
    }
    if (text == "se") {
        return TokenKind::If;
    }
    if (text == "altrimenti") {
        return TokenKind::Else;
    }
    if (text == "firmete") {
        return TokenKind::End;
    }
    if (text == "vale" || text == "diventa") {
        return TokenKind::AssignWord;
    }
    if (text == "piu") {
        return TokenKind::Plus;
    }
    if (text == "meno") {
        return TokenKind::Minus;
    }
    if (text == "pe") {
        return TokenKind::Star;
    }
    if (text == "scumpunne") {
        return TokenKind::Slash;
    }
    if (text == "uguale") {
        return TokenKind::EqualEqual;
    }
    if (text == "de") {
        return TokenKind::De;
    }
    if (text == "diverse") {
        return TokenKind::NotEqual;
    }
    if (text == "meno_de") {
        return TokenKind::Less;
    }
    if (text == "meno_uguale") {
        return TokenKind::LessEqual;
    }
    if (text == "piu_de") {
        return TokenKind::Greater;
    }
    if (text == "piu_uguale") {
        return TokenKind::GreaterEqual;
    }
    if (text == "e") {
        return TokenKind::And;
    }
    if (text == "o") {
        return TokenKind::Or;
    }
    if (text == "ne") {
        return TokenKind::Not;
    }
    if (text == "sci" || text == "vero") {
        return TokenKind::True;
    }
    if (text == "no" || text == "falso") {
        return TokenKind::False;
    }
    if (text == "quinde") {
        return TokenKind::Function;
    }
    if (text == "tornete") {
        return TokenKind::Return;
    }
    if (text == "mbe") {
        return TokenKind::Lambda;
    }
    if (text == "po") {
        return TokenKind::Po;
    }
    if (text == "prime") {
        return TokenKind::Prime;
    }
    if (text == "uldeme") {
        return TokenKind::Uldeme;
    }
    if (text == "vute") {
        return TokenKind::Vute;
    }
    if (text == "mitta") {
        return TokenKind::Mitta;
    }
    if (text == "simele") {
        return TokenKind::Simele;
    }
    if (text == "cusci") {
        return TokenKind::Cusci;
    }
    if (text == "damme") {
        return TokenKind::Damme;
    }
    if (text == "numere") {
        return TokenKind::Numere;
    }
    if (text == "quante") {
        return TokenKind::Quante;
    }
    if (text == "spezza") {
        return TokenKind::Spezza;
    }
    if (text == "iecch") {
        return TokenKind::Iecch;
    }
    if (text == "cala") {
        return TokenKind::Cala;
    }
    if (text == "suva") {
        return TokenKind::Suva;
    }
    if (text == "arretunne") {
        return TokenKind::Arretunne;
    }
    if (text == "radice") {
        return TokenKind::Radice;
    }
    if (text == "quadrata") {
        return TokenKind::Quadrata;
    }
    if (text == "vabbone") {
        return TokenKind::Vabbone;
    }
    if (text == "guaje") {
        return TokenKind::Guaje;
    }
    if (text == "prove") {
        return TokenKind::Prove;
    }
    if (text == "dove") {
        return TokenKind::Dove;
    }
    return TokenKind::Identifier;
}

} // namespace

const char* tokenKindName(TokenKind kind) {
    switch (kind) {
    case TokenKind::EndOfFile:
        return "fine file";
    case TokenKind::Identifier:
        return "identificatore";
    case TokenKind::Number:
        return "numero";
    case TokenKind::String:
        return "stringa";
    case TokenKind::Let:
        return "dichiarazione";
    case TokenKind::Print:
        return "dicce";
    case TokenKind::If:
        return "se";
    case TokenKind::Else:
        return "altrimenti";
    case TokenKind::End:
        return "firmete";
    case TokenKind::True:
        return "sci";
    case TokenKind::False:
        return "no";
    case TokenKind::Plus:
        return "piu";
    case TokenKind::Minus:
        return "-";
    case TokenKind::Star:
        return "*";
    case TokenKind::Slash:
        return "scumpunne";
    case TokenKind::Equal:
        return "=";
    case TokenKind::EqualEqual:
        return "uguale";
    case TokenKind::De:
        return "de";
    case TokenKind::NotEqual:
        return "diverse";
    case TokenKind::Less:
        return "meno_de";
    case TokenKind::LessEqual:
        return "meno_uguale";
    case TokenKind::Greater:
        return "piu_de";
    case TokenKind::GreaterEqual:
        return "piu_uguale";
    case TokenKind::And:
        return "e";
    case TokenKind::Or:
        return "o";
    case TokenKind::Not:
        return "ne";
    case TokenKind::AssignWord:
        return "vale";
    case TokenKind::Semicolon:
        return ";";
    case TokenKind::LeftParen:
        return "(";
    case TokenKind::RightParen:
        return ")";
    case TokenKind::Comma:
        return ",";
    case TokenKind::Function:
        return "quinde";
    case TokenKind::Return:
        return "tornete";
    case TokenKind::Lambda:
        return "mbe";
    case TokenKind::Fine:
        return "fine"; // unused: mbe now closes with firmete
    case TokenKind::Po:
        return "po";
    case TokenKind::LeftBracket:
        return "[";
    case TokenKind::RightBracket:
        return "]";
    case TokenKind::Prime:
        return "prime";
    case TokenKind::Uldeme:
        return "uldeme";
    case TokenKind::Vute:
        return "vute";
    case TokenKind::Mitta:
        return "mitta";
    case TokenKind::Simele:
        return "simele";
    case TokenKind::Cusci:
        return "cusci";
    case TokenKind::Pipe:
        return "|";
    case TokenKind::Damme:
        return "damme";
    case TokenKind::Numere:
        return "numere";
    case TokenKind::Quante:
        return "quante";
    case TokenKind::Percent:
        return "%";
    case TokenKind::Spezza:
        return "spezza";
    case TokenKind::Iecch:
        return "iecch";
    case TokenKind::Cala:
        return "cala";
    case TokenKind::Suva:
        return "suva";
    case TokenKind::Arretunne:
        return "arretunne";
    case TokenKind::Radice:
        return "radice";
    case TokenKind::Quadrata:
        return "quadrata";
    case TokenKind::Vabbone:
        return "vabbone";
    case TokenKind::Guaje:
        return "guaje";
    case TokenKind::Prove:
        return "prove";
    case TokenKind::Dove:
        return "dove";
    }
    return "token sconosciuto";
}

Lexer::Lexer(std::string_view source) : source_(source) {}

Token Lexer::nextToken() {
    const std::size_t posBefore = current_;
    skipTrivia();
    const bool ws = (current_ != posBefore);
    Token tok = nextTokenInner();
    tok.followsWhitespace = ws;
    return tok;
}

Token Lexer::nextTokenInner() {
    const auto start = location_;
    if (isAtEnd()) {
        return makeToken(TokenKind::EndOfFile, "", start);
    }

    const char value = advance();
    switch (value) {
    case '+':
        return makeToken(TokenKind::Plus, "+", start);
    case '-':
        return makeToken(TokenKind::Minus, "-", start);
    case '*':
        return makeToken(TokenKind::Star, "*", start);
    case '/':
        return makeToken(TokenKind::Slash, "/", start);
    case '=':
        if (match('=')) {
            return makeToken(TokenKind::EqualEqual, "==", start);
        }
        return makeToken(TokenKind::Equal, "=", start);
    case '!':
        if (match('=')) {
            return makeToken(TokenKind::NotEqual, "!=", start);
        }
        return makeToken(TokenKind::Not, "!", start);
    case '<':
        if (match('=')) {
            return makeToken(TokenKind::LessEqual, "<=", start);
        }
        return makeToken(TokenKind::Less, "<", start);
    case '>':
        if (match('=')) {
            return makeToken(TokenKind::GreaterEqual, ">=", start);
        }
        return makeToken(TokenKind::Greater, ">", start);
    case ';':
        return makeToken(TokenKind::Semicolon, ";", start);
    case ',':
        return makeToken(TokenKind::Comma, ",", start);
    case '(':
        return makeToken(TokenKind::LeftParen, "(", start);
    case ')':
        return makeToken(TokenKind::RightParen, ")", start);
    case '[':
        return makeToken(TokenKind::LeftBracket, "[", start);
    case ']':
        return makeToken(TokenKind::RightBracket, "]", start);
    case '|':
        return makeToken(TokenKind::Pipe, "|", start);
    case '%':
        return makeToken(TokenKind::Percent, "%", start);
    case '"':
        return string(start);
    default:
        break;
    }

    if (std::isdigit(static_cast<unsigned char>(value))) {
        return number(start);
    }

    if (isIdentifierStart(value)) {
        return identifier(start);
    }

    throw DiagnosticError(start, std::string("carattere inatteso '") + value + "'");
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.size();
}

char Lexer::peek() const {
    if (isAtEnd()) {
        return '\0';
    }
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.size()) {
        return '\0';
    }
    return source_[current_ + 1];
}

char Lexer::advance() {
    const char value = source_[current_++];
    if (value == '\n') {
        location_.line += 1;
        location_.column = 1;
    } else {
        location_.column += 1;
    }
    return value;
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::skipTrivia() {
    while (!isAtEnd()) {
        const char value = peek();
        if (value == ' ' || value == '\r' || value == '\t' || value == '\n') {
            advance();
            continue;
        }

        if (value == '#') {
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
            continue;
        }

        break;
    }
}

Token Lexer::makeToken(TokenKind kind, std::string lexeme, SourceLocation location) const {
    return Token{kind, std::move(lexeme), location};
}

Token Lexer::identifier(SourceLocation start) {
    const std::size_t startOffset = current_ - 1;
    while (isIdentifierPart(peek())) {
        advance();
    }

    auto text = source_.substr(startOffset, current_ - startOffset);
    return makeToken(keywordKind(text), std::string(text), start);
}

Token Lexer::number(SourceLocation start) {
    const std::size_t startOffset = current_ - 1;
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    auto text = source_.substr(startOffset, current_ - startOffset);
    return makeToken(TokenKind::Number, std::string(text), start);
}

Token Lexer::string(SourceLocation start) {
    std::string value;

    while (!isAtEnd() && peek() != '"') {
        const char current = advance();
        if (current == '\\') {
            if (isAtEnd()) {
                throw DiagnosticError(start, "stringa non terminata");
            }

            const char escaped = advance();
            switch (escaped) {
            case 'n':
                value.push_back('\n');
                break;
            case 't':
                value.push_back('\t');
                break;
            case '"':
                value.push_back('"');
                break;
            case '\\':
                value.push_back('\\');
                break;
            default:
                throw DiagnosticError(location_, std::string("escape non valido \\") + escaped);
            }
            continue;
        }

        value.push_back(current);
    }

    if (isAtEnd()) {
        throw DiagnosticError(start, "stringa non terminata");
    }

    advance();
    return makeToken(TokenKind::String, std::move(value), start);
}

} // namespace scioh
