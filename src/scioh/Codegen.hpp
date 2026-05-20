#pragma once

#include "scioh/Ast.hpp"
#include "scioh/Type.hpp"

#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace scioh {

class Codegen {
public:
    void emit(const Program& program, std::ostream& out,
              const std::unordered_map<std::string, TyPtr>& fnTypes = {});

private:
    struct Symbol {
        std::string sourceName;
        std::string cppName;
    };

    // --- Value-based emission (unchanged path) ---
    void emitStatement(const Stmt& stmt, std::ostream& out, int indentLevel);
    void emitStatements(const std::vector<std::unique_ptr<Stmt>>& statements, std::ostream& out, int indentLevel);
    void emitBranchBody(const std::vector<std::unique_ptr<Stmt>>& stmts, std::ostream& out, int indentLevel);
    void emitTailBranchBody(const std::vector<std::unique_ptr<Stmt>>& stmts, std::ostream& out, int indentLevel);
    void emitTailReturn(const Expr& expr, std::ostream& out, int indentLevel);
    void emitMatchBranches(const MatchExpr& matchExpr, std::ostream& out, int guardIndent, int bodyIndent, bool isTail);
    std::string emitExpr(const Expr& expr);

    // --- Typed emission path ---
    struct TypedSym { std::string cppName; TyPtr ty; };
    using TypedEnv = std::unordered_map<std::string, TypedSym>;

    static std::string nativeType(const TyPtr& ty);
    // Returns native C++ expression; empty string if node can't be typed
    std::string emitTypedExpr(const Expr& expr, const TypedEnv& tenv);
    // Emits intermediate typed let stmts; returns the final result expression
    std::string emitTypedBody(const std::vector<std::unique_ptr<Stmt>>& stmts,
                              std::ostream& out, int indent, TypedEnv tenv);
    // Emits a static typed C++ function above main()
    void emitTypedFunction(const FunctionStmt& fn, const TyPtr& fnTy, std::ostream& out);
    // True if fn has a fully-primitive typed version registered
    bool isTyped(const std::string& name) const;

    // --- symbol table ---
    std::string symbolFor(const std::string& name, SourceLocation location) const;
    std::string declareSymbol(const std::string& name, SourceLocation location);
    void declareFunctionSymbol(const std::string& name);
    void pushScope();
    void popScope();

    std::vector<std::vector<Symbol>> scopes_;
    std::unordered_map<std::string, std::string> functionSymbols_;
    std::size_t nextSymbol_ = 0;
    std::size_t nextTypedSym_ = 0;

    // functions with known all-primitive monomorphic types
    std::unordered_map<std::string, TyPtr> typedFns_;
};

} // namespace scioh
