#pragma once

#include "scioh/SourceLocation.hpp"
#include "scioh/Token.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace scioh {

enum class ExprKind {
    Number,
    String,
    Boolean,
    Identifier,
    Unary,
    Binary,
    Call,
    If,
    Lambda,
    List,
    Match,
    Input,
    Result,
    Prove,
};

struct Expr {
    explicit Expr(ExprKind kind, SourceLocation location) : kind(kind), location(location) {}
    virtual ~Expr() = default;

    ExprKind kind;
    SourceLocation location;
};

struct NumberExpr final : Expr {
    NumberExpr(std::string value, SourceLocation location)
        : Expr(ExprKind::Number, location), value(std::move(value)) {}

    std::string value;
};

struct StringExpr final : Expr {
    StringExpr(std::string value, SourceLocation location)
        : Expr(ExprKind::String, location), value(std::move(value)) {}

    std::string value;
};

struct BooleanExpr final : Expr {
    BooleanExpr(bool value, SourceLocation location)
        : Expr(ExprKind::Boolean, location), value(value) {}

    bool value;
};

struct IdentifierExpr final : Expr {
    IdentifierExpr(std::string name, SourceLocation location)
        : Expr(ExprKind::Identifier, location), name(std::move(name)) {}

    std::string name;
};

struct UnaryExpr final : Expr {
    UnaryExpr(TokenKind op, std::unique_ptr<Expr> right, SourceLocation location)
        : Expr(ExprKind::Unary, location), op(op), right(std::move(right)) {}

    TokenKind op;
    std::unique_ptr<Expr> right;
};

struct BinaryExpr final : Expr {
    BinaryExpr(TokenKind op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, SourceLocation location)
        : Expr(ExprKind::Binary, location), op(op), left(std::move(left)), right(std::move(right)) {}

    TokenKind op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

struct CallExpr final : Expr {
    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args, SourceLocation location)
        : Expr(ExprKind::Call, location), callee(std::move(callee)), args(std::move(args)) {}

    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
};

struct ResultExpr final : Expr {
    ResultExpr(bool isOk, std::unique_ptr<Expr> value, SourceLocation location)
        : Expr(ExprKind::Result, location), isOk(isOk), value(std::move(value)) {}
    bool isOk;
    std::unique_ptr<Expr> value;
};

struct ProveExpr final : Expr {
    ProveExpr(std::unique_ptr<Expr> body, SourceLocation location)
        : Expr(ExprKind::Prove, location), body(std::move(body)) {}
    std::unique_ptr<Expr> body;
};

struct InputExpr final : Expr {
    explicit InputExpr(SourceLocation location) : Expr(ExprKind::Input, location) {}
};

struct ListExpr final : Expr {
    ListExpr(std::vector<std::unique_ptr<Expr>> elements, SourceLocation location)
        : Expr(ExprKind::List, location), elements(std::move(elements)) {}

    std::vector<std::unique_ptr<Expr>> elements;
};

struct LambdaExpr final : Expr {
    LambdaExpr(std::vector<std::string> params, std::unique_ptr<Expr> body, SourceLocation location)
        : Expr(ExprKind::Lambda, location), params(std::move(params)), body(std::move(body)) {}

    std::vector<std::string> params;
    std::unique_ptr<Expr> body;
};

// Forward declaration needed for IfExpr branches
struct Stmt;

enum class PatternKind {
    Wildcard,
    Number,
    String,
    Boolean,
    EmptyList,
    Cons,
    ResultOk,
    ResultErr,
};

struct MatchBranch {
    PatternKind patternKind = PatternKind::Wildcard;
    std::string numberLiteral;
    std::string stringLiteral;
    bool boolLiteral = false;
    std::string headName;
    std::string tailName;
    std::vector<std::unique_ptr<Stmt>> body;
};

struct MatchExpr final : Expr {
    MatchExpr(std::unique_ptr<Expr> subject, std::vector<MatchBranch> branches, SourceLocation location)
        : Expr(ExprKind::Match, location), subject(std::move(subject)), branches(std::move(branches)) {}

    std::unique_ptr<Expr> subject;
    std::vector<MatchBranch> branches;
};

struct IfExpr final : Expr {
    IfExpr(std::unique_ptr<Expr> condition,
           std::vector<std::unique_ptr<Stmt>> thenBranch,
           std::vector<std::unique_ptr<Stmt>> elseBranch,
           SourceLocation location)
        : Expr(ExprKind::If, location), condition(std::move(condition)),
          thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> thenBranch;
    std::vector<std::unique_ptr<Stmt>> elseBranch;
};

enum class StmtKind {
    Let,
    Print,
    Function,
    Return,
    ExprStmt,
};

struct Stmt {
    explicit Stmt(StmtKind kind, SourceLocation location) : kind(kind), location(location) {}
    virtual ~Stmt() = default;

    StmtKind kind;
    SourceLocation location;
};

struct LetStmt final : Stmt {
    LetStmt(std::string name, std::unique_ptr<Expr> value, SourceLocation location)
        : Stmt(StmtKind::Let, location), name(std::move(name)), value(std::move(value)) {}

    std::string name;
    std::unique_ptr<Expr> value;
};

struct PrintStmt final : Stmt {
    PrintStmt(std::unique_ptr<Expr> value, SourceLocation location)
        : Stmt(StmtKind::Print, location), value(std::move(value)) {}

    std::unique_ptr<Expr> value;
};

struct FunctionStmt final : Stmt {
    FunctionStmt(std::string name, std::vector<std::string> params,
                 std::vector<std::unique_ptr<Stmt>> body, SourceLocation location)
        : Stmt(StmtKind::Function, location), name(std::move(name)),
          params(std::move(params)), body(std::move(body)) {}

    std::string name;
    std::vector<std::string> params;
    std::vector<std::unique_ptr<Stmt>> body;
};

struct ReturnStmt final : Stmt {
    ReturnStmt(std::unique_ptr<Expr> value, SourceLocation location)
        : Stmt(StmtKind::Return, location), value(std::move(value)) {}

    std::unique_ptr<Expr> value;
};

struct ExprStmt final : Stmt {
    ExprStmt(std::unique_ptr<Expr> expr, SourceLocation location)
        : Stmt(StmtKind::ExprStmt, location), expr(std::move(expr)) {}

    std::unique_ptr<Expr> expr;
};

struct Program {
    std::vector<std::unique_ptr<Stmt>> statements;
};

} // namespace scioh
