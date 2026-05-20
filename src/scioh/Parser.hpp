#pragma once

#include "scioh/Ast.hpp"
#include "scioh/Lexer.hpp"

#include <memory>

namespace scioh {

class Parser {
public:
    explicit Parser(Lexer lexer);

    Program parseProgram();

private:
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> letStatement();
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> functionStatement();
    std::unique_ptr<Stmt> returnStatement();
    std::unique_ptr<Stmt> exprStatement();
    std::vector<std::unique_ptr<Stmt>> statementsUntil(TokenKind firstStop, TokenKind secondStop);
    void finishStatement(const char* message);

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> pipeExpression();
    std::unique_ptr<Expr> consExpression();
    std::unique_ptr<Expr> lambdaExpression();
    std::unique_ptr<Expr> matchExpression();
    std::unique_ptr<Expr> orExpression();
    std::unique_ptr<Expr> andExpression();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> application();
    std::unique_ptr<Expr> primary();
    std::unique_ptr<Expr> ifExpression();
    bool canStartPrimary() const;

    Token consume(TokenKind kind, const char* message);
    bool check(TokenKind kind) const;
    bool checkNext(TokenKind kind) const;
    bool match(TokenKind kind);
    Token advance();

    Lexer lexer_;
    Token current_;
    Token next_;
};

} // namespace scioh
