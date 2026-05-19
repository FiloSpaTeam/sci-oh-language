#pragma once

#include "scioh/Ast.hpp"

#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace scioh {

class Codegen {
public:
    void emit(const Program& program, std::ostream& out);

private:
    struct Symbol {
        std::string sourceName;
        std::string cppName;
    };

    void emitStatement(const Stmt& stmt, std::ostream& out, int indentLevel);
    void emitStatements(const std::vector<std::unique_ptr<Stmt>>& statements, std::ostream& out, int indentLevel);
    // For IIFE branch bodies: last ExprStmt → return expr.
    void emitBranchBody(const std::vector<std::unique_ptr<Stmt>>& stmts, std::ostream& out, int indentLevel);
    // For tail-return contexts: last ExprStmt → emitTailReturn.
    void emitTailBranchBody(const std::vector<std::unique_ptr<Stmt>>& stmts, std::ostream& out, int indentLevel);
    // Emit expr in tail-return position: CallExpr → throw TailCall, If/Match → plain if-else, other → throw ReturnValue.
    void emitTailReturn(const Expr& expr, std::ostream& out, int indentLevel);
    // Shared helper: emits the if/else-if chain for all match branches.
    void emitMatchBranches(const MatchExpr& matchExpr, std::ostream& out, int guardIndent, int bodyIndent, bool isTail);
    std::string emitExpr(const Expr& expr);
    std::string symbolFor(const std::string& name, SourceLocation location) const;
    std::string declareSymbol(const std::string& name, SourceLocation location);
    void declareFunctionSymbol(const std::string& name);
    void pushScope();
    void popScope();

    std::vector<std::vector<Symbol>> scopes_;
    std::unordered_map<std::string, std::string> functionSymbols_; // name → "fn_"+name
    std::size_t nextSymbol_ = 0;
};

} // namespace scioh
