#include "scioh/Parser.hpp"

#include "scioh/Diagnostic.hpp"

#include <utility>

namespace scioh {

Parser::Parser(Lexer lexer)
    : lexer_(std::move(lexer)), current_(lexer_.nextToken()), next_(lexer_.nextToken()) {}

Program Parser::parseProgram() {
    Program program;
    while (!check(TokenKind::EndOfFile)) {
        program.statements.push_back(statement());
    }
    return program;
}

std::unique_ptr<Stmt> Parser::statement() {
    if (check(TokenKind::Let)) {
        return letStatement();
    }
    if (check(TokenKind::Print)) {
        return printStatement();
    }
    if (check(TokenKind::Function)) {
        return functionStatement();
    }
    if (check(TokenKind::Return)) {
        return returnStatement();
    }
    if (check(TokenKind::Dove)) {
        throw DiagnosticError(current_.location, "'dove' è valido solo alla fine de quinde");
    }
    // Identifier followed by AssignWord or Equal → immutability error
    if (check(TokenKind::Identifier) &&
        (checkNext(TokenKind::AssignWord) || checkNext(TokenKind::Equal))) {
        const auto tok = advance(); // consume identifier
        advance(); // consume vale/=
        throw DiagnosticError(tok.location,
            "le variabili so' immutabili: ne se po' riassegna' '" + tok.lexeme + "' doppe la dichiarazione");
    }
    // Everything else (including ie expressions, calls, etc.) goes to exprStatement
    return exprStatement();
}

std::unique_ptr<Stmt> Parser::letStatement() {
    const auto start = advance().location;
    auto name = consume(TokenKind::Identifier, "mi aspettavo il nome della variabile").lexeme;
    if (check(TokenKind::Equal) || check(TokenKind::AssignWord)) {
        advance();
    }
    auto value = expression();
    finishStatement("mi aspettavo la fine della dichiarazione");
    return std::make_unique<LetStmt>(std::move(name), std::move(value), start);
}

std::unique_ptr<Stmt> Parser::printStatement() {
    const auto start = advance().location;
    auto value = expression();
    finishStatement("mi aspettavo la fine de dicce");
    return std::make_unique<PrintStmt>(std::move(value), start);
}

std::unique_ptr<Stmt> Parser::functionStatement() {
    const auto start = advance().location;
    auto name = consume(TokenKind::Identifier, "mi aspettavo lu nome della funzione").lexeme;
    consume(TokenKind::LeftParen, "mi aspettavo '(' dopo lu nome della funzione");

    std::vector<std::string> params;
    if (!check(TokenKind::RightParen)) {
        params.push_back(consume(TokenKind::Identifier, "mi aspettavo lu nome del parametro").lexeme);
        while (match(TokenKind::Comma)) {
            params.push_back(consume(TokenKind::Identifier, "mi aspettavo lu nome del parametro").lexeme);
        }
    }
    consume(TokenKind::RightParen, "mi aspettavo ')' dopo li parametri");

    // Parse body, stopping at dove or firmete
    std::vector<std::unique_ptr<Stmt>> body;
    while (!check(TokenKind::EndOfFile) && !check(TokenKind::End) && !check(TokenKind::Dove)) {
        body.push_back(statement());
    }

    // Parse dove bindings, prepend them to the body in declared order
    if (check(TokenKind::Dove)) {
        std::vector<std::unique_ptr<Stmt>> doveStmts;
        while (check(TokenKind::Dove)) {
            const auto doveLoc = advance().location;
            auto doveName = consume(TokenKind::Identifier, "mi aspettavo lu nome dopo dove").lexeme;
            if (check(TokenKind::AssignWord) || check(TokenKind::Equal)) {
                advance();
            }
            auto doveValue = expression();
            finishStatement("mi aspettavo la fine de dove");
            doveStmts.push_back(std::make_unique<LetStmt>(std::move(doveName), std::move(doveValue), doveLoc));
        }
        // Prepend in forward order so the last dove is emitted first:
        // the user writes compound-first, building-blocks-last (Haskell where style),
        // and building blocks need to be evaluated before the things that use them.
        for (auto it = doveStmts.begin(); it != doveStmts.end(); ++it) {
            body.insert(body.begin(), std::move(*it));
        }
    }

    consume(TokenKind::End, "mi aspettavo 'firmete' alla fine de quinde");

    return std::make_unique<FunctionStmt>(std::move(name), std::move(params), std::move(body), start);
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    const auto start = advance().location;
    // 'a' opzionale come particella dialettale: "tornete a valore"
    if (check(TokenKind::Identifier) && current_.lexeme == "a") {
        advance();
    }
    auto value = expression();
    finishStatement("mi aspettavo la fine de tornete");
    return std::make_unique<ReturnStmt>(std::move(value), start);
}

std::unique_ptr<Stmt> Parser::exprStatement() {
    const auto start = current_.location;
    auto expr = expression();
    finishStatement("mi aspettavo la fine della chiamata");
    return std::make_unique<ExprStmt>(std::move(expr), start);
}

std::vector<std::unique_ptr<Stmt>> Parser::statementsUntil(TokenKind firstStop, TokenKind secondStop) {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenKind::EndOfFile) && !check(firstStop) && !check(secondStop)) {
        statements.push_back(statement());
    }
    return statements;
}

void Parser::finishStatement(const char* message) {
    if (check(TokenKind::Semicolon)) {
        advance();
        return;
    }

    // ie is a self-terminating expression (ends with firmete), so after
    // firmete the next token is a valid statement boundary.
    if (check(TokenKind::EndOfFile) || check(TokenKind::Let) || check(TokenKind::Print) ||
        check(TokenKind::Else) || check(TokenKind::End) ||
        check(TokenKind::Function) || check(TokenKind::Return) || check(TokenKind::Identifier) ||
        check(TokenKind::If) || check(TokenKind::Not) || check(TokenKind::Lambda) ||
        check(TokenKind::Po) || check(TokenKind::Prime) || check(TokenKind::Uldeme) ||
        check(TokenKind::Vute) || check(TokenKind::LeftBracket) ||
        check(TokenKind::Simele) || check(TokenKind::Cusci) ||
        check(TokenKind::Damme) || check(TokenKind::Numere) || check(TokenKind::Quante) ||
        check(TokenKind::Cala) || check(TokenKind::Suva) || check(TokenKind::Arretunne) ||
        check(TokenKind::Radice) || check(TokenKind::Vabbone) || check(TokenKind::Guaje) ||
        check(TokenKind::Prove) || check(TokenKind::Dove)) {
        return;
    }

    throw DiagnosticError(current_.location, message);
}

std::unique_ptr<Expr> Parser::expression() {
    return consExpression();
}

std::unique_ptr<Expr> Parser::consExpression() {
    auto expr = orExpression();

    if (check(TokenKind::Mitta) && checkNext(TokenKind::Prime)) {
        const auto op = advance(); // consume mitta
        advance();                 // consume prim
        auto right = consExpression();
        return std::make_unique<BinaryExpr>(TokenKind::Mitta, std::move(expr), std::move(right), op.location);
    }

    if (check(TokenKind::Spezza) && checkNext(TokenKind::Iecch)) {
        const auto op = advance(); // consume spezza
        advance();                 // consume iecch
        auto right = orExpression();
        return std::make_unique<BinaryExpr>(TokenKind::Spezza, std::move(expr), std::move(right), op.location);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::lambdaExpression() {
    const auto start = advance().location; // consume 'mbe'
    consume(TokenKind::LeftParen, "mi aspettavo '(' dopo mbe");

    std::vector<std::string> params;
    if (!check(TokenKind::RightParen)) {
        params.push_back(consume(TokenKind::Identifier, "mi aspettavo lu nome del parametro").lexeme);
        while (match(TokenKind::Comma)) {
            params.push_back(consume(TokenKind::Identifier, "mi aspettavo lu nome del parametro").lexeme);
        }
    }
    consume(TokenKind::RightParen, "mi aspettavo ')' dopo li parametri");

    auto body = expression();
    consume(TokenKind::End, "mi aspettavo 'firmete' alla fine de mbe");

    return std::make_unique<LambdaExpr>(std::move(params), std::move(body), start);
}

std::unique_ptr<Expr> Parser::orExpression() {
    auto expr = andExpression();

    while (check(TokenKind::Or)) {
        const auto op = advance();
        auto right = andExpression();
        expr = std::make_unique<BinaryExpr>(op.kind, std::move(expr), std::move(right), op.location);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::andExpression() {
    auto expr = equality();

    while (check(TokenKind::And)) {
        const auto op = advance();
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(op.kind, std::move(expr), std::move(right), op.location);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();

    while (check(TokenKind::EqualEqual) || check(TokenKind::NotEqual)) {
        const auto op = advance();
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(op.kind, std::move(expr), std::move(right), op.location);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();

    while (true) {
        if (check(TokenKind::Plus) && checkNext(TokenKind::De)) {
            const auto op = advance();
            advance();
            auto right = term();
            expr = std::make_unique<BinaryExpr>(TokenKind::Greater, std::move(expr), std::move(right), op.location);
            continue;
        }

        if (check(TokenKind::Plus) && checkNext(TokenKind::EqualEqual)) {
            const auto op = advance();
            advance();
            auto right = term();
            expr = std::make_unique<BinaryExpr>(TokenKind::GreaterEqual, std::move(expr), std::move(right), op.location);
            continue;
        }

        if (check(TokenKind::Minus) && checkNext(TokenKind::De)) {
            const auto op = advance();
            advance();
            auto right = term();
            expr = std::make_unique<BinaryExpr>(TokenKind::Less, std::move(expr), std::move(right), op.location);
            continue;
        }

        if (check(TokenKind::Minus) && checkNext(TokenKind::EqualEqual)) {
            const auto op = advance();
            advance();
            auto right = term();
            expr = std::make_unique<BinaryExpr>(TokenKind::LessEqual, std::move(expr), std::move(right), op.location);
            continue;
        }

        if (check(TokenKind::Greater) || check(TokenKind::GreaterEqual) || check(TokenKind::Less) ||
            check(TokenKind::LessEqual)) {
            const auto op = advance();
            auto right = term();
            expr = std::make_unique<BinaryExpr>(op.kind, std::move(expr), std::move(right), op.location);
            continue;
        }

        if (check(TokenKind::EqualEqual) || check(TokenKind::NotEqual)) {
            const auto op = advance();
            auto right = term();
            expr = std::make_unique<BinaryExpr>(op.kind, std::move(expr), std::move(right), op.location);
            continue;
        }

        break;
    }

    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();

    while ((check(TokenKind::Plus) || check(TokenKind::Minus)) &&
           !(checkNext(TokenKind::De) || checkNext(TokenKind::EqualEqual))) {
        const auto op = advance();
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(op.kind, std::move(expr), std::move(right), op.location);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = unary();

    while (check(TokenKind::Star) || check(TokenKind::Slash) || check(TokenKind::Percent)) {
        const auto op = advance();
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(op.kind, std::move(expr), std::move(right), op.location);
    }

    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (check(TokenKind::Not) || check(TokenKind::Minus) ||
        check(TokenKind::Prime) || check(TokenKind::Uldeme) || check(TokenKind::Vute) ||
        check(TokenKind::Numere) || check(TokenKind::Quante) ||
        check(TokenKind::Cala) || check(TokenKind::Suva) || check(TokenKind::Arretunne)) {
        const auto op = advance();
        auto right = unary();
        return std::make_unique<UnaryExpr>(op.kind, std::move(right), op.location);
    }

    if (check(TokenKind::Radice) && checkNext(TokenKind::Quadrata)) {
        const auto op = advance(); // consume radice
        advance();                 // consume quadrata
        auto right = unary();
        return std::make_unique<UnaryExpr>(TokenKind::Radice, std::move(right), op.location);
    }

    return application();
}

bool Parser::canStartPrimary() const {
    return check(TokenKind::Number)       ||
           check(TokenKind::String)       ||
           check(TokenKind::True)         ||
           check(TokenKind::False)        ||
           check(TokenKind::Identifier)   ||
           check(TokenKind::LeftParen)    ||
           check(TokenKind::LeftBracket);
}

// Function call dispatch:
//   f(x, y)   — '(' immediately after name (no whitespace) → old explicit syntax
//   f x y z   — space-separated args on same line → Haskell-style juxtaposition
// The two forms are mutually exclusive based on whether '(' follows whitespace.
std::unique_ptr<Expr> Parser::application() {
    auto callee = primary();

    if (callee->kind != ExprKind::Identifier) {
        return callee;
    }

    const auto loc = callee->location;

    // Old explicit call: f(a, b, c)  — '(' with no preceding whitespace
    if (check(TokenKind::LeftParen) && !current_.followsWhitespace) {
        advance(); // consume '('
        std::vector<std::unique_ptr<Expr>> args;
        if (!check(TokenKind::RightParen)) {
            args.push_back(expression());
            while (match(TokenKind::Comma)) {
                args.push_back(expression());
            }
        }
        consume(TokenKind::RightParen, "mi aspettavo ')' dopo li argomenti");
        return std::make_unique<CallExpr>(std::move(callee), std::move(args), loc);
    }

    // New juxtaposition call: f x y z — same line, whitespace-separated
    if (!canStartPrimary() || current_.location.line != loc.line) {
        return callee;
    }
    std::vector<std::unique_ptr<Expr>> args;
    while (canStartPrimary() && current_.location.line == loc.line) {
        args.push_back(primary());
    }
    return std::make_unique<CallExpr>(std::move(callee), std::move(args), loc);
}

std::unique_ptr<Expr> Parser::primary() {
    if (check(TokenKind::Number)) {
        auto token = advance();
        return std::make_unique<NumberExpr>(std::move(token.lexeme), token.location);
    }

    if (check(TokenKind::String)) {
        auto token = advance();
        return std::make_unique<StringExpr>(std::move(token.lexeme), token.location);
    }

    if (check(TokenKind::True)) {
        auto token = advance();
        return std::make_unique<BooleanExpr>(true, token.location);
    }

    if (check(TokenKind::False)) {
        auto token = advance();
        return std::make_unique<BooleanExpr>(false, token.location);
    }

    // ie is now an expression
    if (check(TokenKind::If)) {
        return ifExpression();
    }

    if (check(TokenKind::Lambda)) {
        return lambdaExpression();
    }

    if (check(TokenKind::Simele)) {
        return matchExpression();
    }

    if (check(TokenKind::Damme)) {
        auto token = advance();
        return std::make_unique<InputExpr>(token.location);
    }

    if (check(TokenKind::Vabbone)) {
        const auto start = advance().location;
        consume(TokenKind::LeftParen, "mi aspettavo '(' dopo vabbone");
        auto value = expression();
        consume(TokenKind::RightParen, "mi aspettavo ')' dopo vabbone(...)");
        return std::make_unique<ResultExpr>(true, std::move(value), start);
    }

    if (check(TokenKind::Guaje)) {
        const auto start = advance().location;
        consume(TokenKind::LeftParen, "mi aspettavo '(' dopo guaje");
        auto value = expression();
        consume(TokenKind::RightParen, "mi aspettavo ')' dopo guaje(...)");
        return std::make_unique<ResultExpr>(false, std::move(value), start);
    }

    if (check(TokenKind::Prove)) {
        const auto start = advance().location;
        auto body = unary();
        return std::make_unique<ProveExpr>(std::move(body), start);
    }

    if (check(TokenKind::Identifier)) {
        auto token = advance();
        return std::make_unique<IdentifierExpr>(std::move(token.lexeme), token.location);
    }

    if (check(TokenKind::LeftBracket)) {
        const auto start = current_.location;
        advance();
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenKind::RightBracket)) {
            elements.push_back(expression());
            while (match(TokenKind::Comma)) {
                elements.push_back(expression());
            }
        }
        consume(TokenKind::RightBracket, "mi aspettavo ']' dopo li elementi della lista");
        return std::make_unique<ListExpr>(std::move(elements), start);
    }

    if (match(TokenKind::LeftParen)) {
        auto expr = expression();
        consume(TokenKind::RightParen, "mi aspettavo ')' dopo l'espressione");
        return expr;
    }

    throw DiagnosticError(current_.location, std::string("mi aspettavo un'espressione, ho trovato ") + tokenKindName(current_.kind));
}

std::unique_ptr<Expr> Parser::ifExpression() {
    const auto start = advance().location; // consume 'se'
    auto condition = expression();
    consume(TokenKind::Po, "mi aspettavo 'po' dopo la condizione de se");
    auto thenBranch = statementsUntil(TokenKind::Else, TokenKind::End);
    consume(TokenKind::Else, "mi aspettavo 'altrimenti' — la se funzionale vole sempe lu ramo altrimenti");
    auto elseBranch = statementsUntil(TokenKind::End, TokenKind::End);
    consume(TokenKind::End, "mi aspettavo 'firmete' alla fine de ie");
    return std::make_unique<IfExpr>(std::move(condition), std::move(thenBranch), std::move(elseBranch), start);
}

Token Parser::consume(TokenKind kind, const char* message) {
    if (check(kind)) {
        return advance();
    }
    throw DiagnosticError(current_.location, message);
}

bool Parser::check(TokenKind kind) const {
    return current_.kind == kind;
}

bool Parser::checkNext(TokenKind kind) const {
    return next_.kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (!check(kind)) {
        return false;
    }
    advance();
    return true;
}

Token Parser::advance() {
    auto previous = current_;
    current_ = next_;
    next_ = lexer_.nextToken();
    return previous;
}

std::unique_ptr<Expr> Parser::matchExpression() {
    const auto start = advance().location; // consume 'simele'
    auto subject = expression();

    std::vector<MatchBranch> branches;

    while (check(TokenKind::Cusci)) {
        advance(); // consume 'cusci'

        MatchBranch branch;

        if (check(TokenKind::Star)) {
            advance();
            branch.patternKind = PatternKind::Wildcard;
        } else if (check(TokenKind::LeftBracket)) {
            advance(); // consume '['
            if (check(TokenKind::RightBracket)) {
                advance();
                branch.patternKind = PatternKind::EmptyList;
            } else {
                branch.headName = consume(TokenKind::Identifier, "mi aspettavo lu nome della testa").lexeme;
                consume(TokenKind::Pipe, "mi aspettavo '|' nello schema della lista");
                branch.tailName = consume(TokenKind::Identifier, "mi aspettavo lu nome della coda").lexeme;
                consume(TokenKind::RightBracket, "mi aspettavo ']' nello schema");
                branch.patternKind = PatternKind::Cons;
            }
        } else if (check(TokenKind::Number)) {
            branch.numberLiteral = advance().lexeme;
            branch.patternKind = PatternKind::Number;
        } else if (check(TokenKind::String)) {
            branch.stringLiteral = advance().lexeme;
            branch.patternKind = PatternKind::String;
        } else if (check(TokenKind::True)) {
            advance();
            branch.boolLiteral = true;
            branch.patternKind = PatternKind::Boolean;
        } else if (check(TokenKind::False)) {
            advance();
            branch.boolLiteral = false;
            branch.patternKind = PatternKind::Boolean;
        } else if (check(TokenKind::Vabbone)) {
            advance();
            consume(TokenKind::LeftParen, "mi aspettavo '(' dopo vabbone nello schema");
            branch.headName = consume(TokenKind::Identifier, "mi aspettavo lu nome da lega'").lexeme;
            consume(TokenKind::RightParen, "mi aspettavo ')' dopo vabbone(...) nello schema");
            branch.patternKind = PatternKind::ResultOk;
        } else if (check(TokenKind::Guaje)) {
            advance();
            consume(TokenKind::LeftParen, "mi aspettavo '(' dopo guaje nello schema");
            branch.headName = consume(TokenKind::Identifier, "mi aspettavo lu nome da lega'").lexeme;
            consume(TokenKind::RightParen, "mi aspettavo ')' dopo guaje(...) nello schema");
            branch.patternKind = PatternKind::ResultErr;
        } else {
            throw DiagnosticError(current_.location,
                "mi aspettavo uno schema: *, [], [h | t], vabbone(x), guaje(x), numero, stringa, sci oppure no");
        }

        consume(TokenKind::Po, "mi aspettavo 'po' dopo lo schema");
        branch.body = statementsUntil(TokenKind::Cusci, TokenKind::End);
        branches.push_back(std::move(branch));
    }

    consume(TokenKind::End, "mi aspettavo 'firmete' alla fine de simele");
    return std::make_unique<MatchExpr>(std::move(subject), std::move(branches), start);
}

} // namespace scioh
