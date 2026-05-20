#pragma once

#include "scioh/Ast.hpp"
#include "scioh/Type.hpp"

#include <string>
#include <unordered_map>

namespace scioh {

// Monomorphic Hindley-Milner type inference.
// Annotates every Expr node's `ty` field in place.
// Best-effort: unknown types stay null (fall back to Scioh::Value in codegen).
class Infer {
public:
    void run(Program& program);

    // After run(): maps function name → its inferred Fun type (or null)
    const std::unordered_map<std::string, TyPtr>& functionTypes() const { return fnTypes_; }

private:
    using Env   = std::unordered_map<std::string, TyPtr>;
    using Subst = std::unordered_map<int, TyPtr>;

    TyPtr fresh();
    TyPtr apply(const TyPtr& ty) const;
    bool  unify(TyPtr a, TyPtr b);
    bool  occursIn(int v, const TyPtr& ty) const;

    TyPtr inferExpr(Expr& expr, const Env& env);
    void  inferStmt(Stmt& stmt, Env& env);
    // Infer the "result type" of a branch body (last ExprStmt or ReturnStmt)
    TyPtr inferBranchResult(const std::vector<std::unique_ptr<Stmt>>& stmts, const Env& env);

    Subst subst_;
    int   nextVar_ = 0;

    std::unordered_map<std::string, TyPtr> fnTypes_;
};

} // namespace scioh
