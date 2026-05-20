#include "scioh/Infer.hpp"

#include <atomic>

namespace scioh {

// --- Ty::freshVar (global counter, used by Type.hpp) ---

TyPtr Ty::freshVar() {
    static std::atomic<int> counter{0};
    auto t = std::make_shared<Ty>(Ty{K::Var});
    t->var = counter.fetch_add(1, std::memory_order_relaxed);
    return t;
}

std::string tyName(const TyPtr& ty) {
    if (!ty) return "?";
    switch (ty->k) {
    case Ty::K::Num:    return "Num";
    case Ty::K::Bool:   return "Bool";
    case Ty::K::Str:    return "Str";
    case Ty::K::List:   return "Lista(" + tyName(ty->inner[0]) + ")";
    case Ty::K::Result: return "Risultato(" + tyName(ty->inner[0]) + ")";
    case Ty::K::Fun: {
        std::string s = "(";
        for (std::size_t i = 0; i + 1 < ty->inner.size(); ++i) {
            if (i) s += ", ";
            s += tyName(ty->inner[i]);
        }
        return s + ") -> " + tyName(ty->inner.back());
    }
    case Ty::K::Var: return "t" + std::to_string(ty->var);
    }
    return "?";
}

// --- Infer internals ---

TyPtr Infer::fresh() {
    auto t = std::make_shared<Ty>(Ty{Ty::K::Var});
    t->var = nextVar_++;
    return t;
}

// Walk the substitution chain for a type variable
TyPtr Infer::apply(const TyPtr& ty) const {
    if (!ty) return ty;
    if (ty->k == Ty::K::Var) {
        auto it = subst_.find(ty->var);
        if (it != subst_.end()) return apply(it->second);
        return ty;
    }
    if (ty->inner.empty()) return ty;
    auto result = std::make_shared<Ty>(Ty{ty->k, ty->var, {}});
    for (const auto& inner : ty->inner)
        result->inner.push_back(apply(inner));
    return result;
}

bool Infer::occursIn(int v, const TyPtr& ty) const {
    if (!ty) return false;
    if (ty->k == Ty::K::Var) return ty->var == v;
    for (const auto& inner : ty->inner)
        if (occursIn(v, inner)) return true;
    return false;
}

bool Infer::unify(TyPtr a, TyPtr b) {
    a = apply(a); b = apply(b);
    if (!a || !b) return false;

    if (a->k == Ty::K::Var) {
        if (b->k == Ty::K::Var && a->var == b->var) return true;
        if (occursIn(a->var, b)) return false;
        subst_[a->var] = b;
        return true;
    }
    if (b->k == Ty::K::Var) return unify(b, a);

    if (a->k != b->k) return false;
    if (a->inner.size() != b->inner.size()) return false;
    for (std::size_t i = 0; i < a->inner.size(); ++i)
        if (!unify(a->inner[i], b->inner[i])) return false;
    return true;
}

// --- inferExpr ---

TyPtr Infer::inferExpr(Expr& expr, const Env& env) {
    TyPtr ty;

    switch (expr.kind) {
    case ExprKind::Number:
        ty = Ty::num(); break;

    case ExprKind::String:
        ty = Ty::str(); break;

    case ExprKind::Boolean:
        ty = Ty::bool_(); break;

    case ExprKind::Identifier: {
        const auto& id = static_cast<const IdentifierExpr&>(expr);
        auto it = env.find(id.name);
        if (it != env.end()) ty = apply(it->second);
        break;
    }

    case ExprKind::Unary: {
        auto& u = static_cast<UnaryExpr&>(expr);
        auto inner = inferExpr(*u.right, env);
        if (u.op == TokenKind::Minus) {
            unify(inner, Ty::num());
            ty = Ty::num();
        } else if (u.op == TokenKind::Not) {
            unify(inner, Ty::bool_());
            ty = Ty::bool_();
        } else if (u.op == TokenKind::Cala || u.op == TokenKind::Suva ||
                   u.op == TokenKind::Arretunne || u.op == TokenKind::Radice) {
            ty = Ty::num();
        } else if (u.op == TokenKind::Quante) {
            ty = Ty::num();
        }
        // Prime, Uldeme, Vute → need list type, skip
        break;
    }

    case ExprKind::Binary: {
        auto& b = static_cast<BinaryExpr&>(expr);
        auto lt = inferExpr(*b.left,  env);
        auto rt = inferExpr(*b.right, env);
        switch (b.op) {
        case TokenKind::Plus:
            if ((lt && lt->isStr()) || (rt && rt->isStr())) {
                ty = Ty::str();
            } else if ((lt && lt->k == Ty::K::List) || (rt && rt->k == Ty::K::List)) {
                // list concatenation: leave untyped
            } else {
                // Numeric addition (or type variable): unify both with Num
                if (lt) unify(lt, Ty::num());
                if (rt) unify(rt, Ty::num());
                ty = Ty::num();
            }
            break;
        case TokenKind::Minus:
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:
            unify(lt, Ty::num()); unify(rt, Ty::num());
            ty = Ty::num(); break;
        case TokenKind::Less: case TokenKind::LessEqual:
        case TokenKind::Greater: case TokenKind::GreaterEqual:
            unify(lt, Ty::num()); unify(rt, Ty::num());
            ty = Ty::bool_(); break;
        case TokenKind::EqualEqual: case TokenKind::NotEqual:
            ty = Ty::bool_(); break;
        case TokenKind::And: case TokenKind::Or:
            unify(lt, Ty::bool_()); unify(rt, Ty::bool_());
            ty = Ty::bool_(); break;
        default: break;
        }
        break;
    }

    case ExprKind::Call: {
        auto& call = static_cast<CallExpr&>(expr);
        auto calleeTy = inferExpr(*call.callee, env);
        calleeTy = apply(calleeTy);

        std::vector<TyPtr> argTypes;
        for (auto& arg : call.args)
            argTypes.push_back(inferExpr(*arg, env));

        if (calleeTy && calleeTy->k == Ty::K::Fun) {
            auto& params = calleeTy->inner;
            if (argTypes.size() + 1 == params.size()) {
                for (std::size_t i = 0; i < argTypes.size(); ++i)
                    unify(argTypes[i], params[i]);
                ty = apply(params.back()); // return type
            }
        } else {
            // Unknown callee: result is fresh var
            ty = fresh();
        }
        break;
    }

    case ExprKind::If: {
        auto& ifExpr = static_cast<IfExpr&>(expr);
        auto condTy = inferExpr(*ifExpr.condition, env);
        unify(condTy, Ty::bool_());
        auto thenTy = inferBranchResult(ifExpr.thenBranch, env);
        auto elseTy = inferBranchResult(ifExpr.elseBranch, env);
        if (thenTy && elseTy) {
            unify(thenTy, elseTy);
            ty = apply(thenTy);
        }
        break;
    }

    case ExprKind::Lambda: {
        auto& lambda = static_cast<LambdaExpr&>(expr);
        Env lambdaEnv = env;
        std::vector<TyPtr> paramTys;
        for (const auto& p : lambda.params) {
            auto pv = fresh();
            lambdaEnv[p] = pv;
            paramTys.push_back(pv);
        }
        auto bodyTy = inferExpr(*lambda.body, lambdaEnv);
        if (bodyTy) ty = Ty::fun(paramTys, bodyTy);
        break;
    }

    case ExprKind::List: {
        auto& listExpr = static_cast<ListExpr&>(expr);
        if (listExpr.elements.empty()) {
            ty = Ty::list(fresh());
        } else {
            auto elemTy = inferExpr(*listExpr.elements[0], env);
            for (std::size_t i = 1; i < listExpr.elements.size(); ++i)
                unify(elemTy, inferExpr(*listExpr.elements[i], env));
            if (elemTy) ty = Ty::list(apply(elemTy));
        }
        break;
    }

    case ExprKind::Input:
        ty = Ty::str(); break;

    case ExprKind::Result: {
        auto& r = static_cast<ResultExpr&>(expr);
        auto inner = inferExpr(*r.value, env);
        if (inner) ty = Ty::result(inner);
        break;
    }

    case ExprKind::Prove: {
        auto& p = static_cast<ProveExpr&>(expr);
        auto inner = inferExpr(*p.body, env);
        if (inner) ty = Ty::result(inner);
        break;
    }

    case ExprKind::Match:
        // Pattern matching: skip inference for now (complex)
        break;
    }

    if (ty) ty = apply(ty);
    expr.ty = ty;
    return ty;
}

TyPtr Infer::inferBranchResult(const std::vector<std::unique_ptr<Stmt>>& stmts, const Env& env) {
    Env localEnv = env;
    for (const auto& stmt : stmts) {
        if (stmt->kind == StmtKind::Let) {
            const auto& let = static_cast<const LetStmt&>(*stmt);
            auto vty = inferExpr(*let.value, localEnv);
            if (vty) localEnv[let.name] = vty;
        } else if (stmt->kind == StmtKind::ExprStmt) {
            if (&stmt == &stmts.back())
                return inferExpr(*static_cast<ExprStmt&>(*stmt).expr, localEnv);
        } else if (stmt->kind == StmtKind::Return) {
            return inferExpr(*static_cast<ReturnStmt&>(*stmt).value, localEnv);
        }
    }
    if (!stmts.empty() && stmts.back()->kind == StmtKind::ExprStmt)
        return inferExpr(*static_cast<ExprStmt&>(*stmts.back()).expr, env);
    return nullptr;
}

// --- inferStmt ---

void Infer::inferStmt(Stmt& stmt, Env& env) {
    switch (stmt.kind) {
    case StmtKind::Let: {
        auto& let = static_cast<LetStmt&>(stmt);
        auto ty = inferExpr(*let.value, env);
        if (ty) env[let.name] = apply(ty);
        break;
    }
    case StmtKind::Print: {
        auto& print = static_cast<PrintStmt&>(stmt);
        inferExpr(*print.value, env);
        break;
    }
    case StmtKind::ExprStmt: {
        auto& es = static_cast<ExprStmt&>(stmt);
        inferExpr(*es.expr, env);
        break;
    }
    case StmtKind::Return: {
        auto& ret = static_cast<ReturnStmt&>(stmt);
        inferExpr(*ret.value, env);
        break;
    }
    case StmtKind::Function: {
        auto& fn = static_cast<FunctionStmt&>(stmt);

        // Create fresh type vars for parameters
        Env fnEnv = env;
        std::vector<TyPtr> paramVars;
        for (const auto& p : fn.params) {
            auto pv = fresh();
            fnEnv[p] = pv;
            paramVars.push_back(pv);
        }

        // Register the function in env with a fresh return var (enables recursion)
        auto retVar = fresh();
        auto fnType = Ty::fun(paramVars, retVar);
        // Unify pass-2 vars with pass-1 pre-registration vars so that constraints
        // from mutual-recursive callers (processed before this function) propagate
        // into this function's body analysis.
        if (auto it = env.find(fn.name); it != env.end() && it->second)
            unify(fnType, it->second);
        fnEnv[fn.name] = fnType;
        env[fn.name]   = fnType;

        // Infer each statement in the function body
        TyPtr returnTy;
        for (const auto& s : fn.body) {
            if (s->kind == StmtKind::Return) {
                auto& ret = static_cast<ReturnStmt&>(*s);
                auto ty = inferExpr(*ret.value, fnEnv);
                if (ty) { unify(retVar, ty); returnTy = apply(retVar); }
            } else {
                inferStmt(*s, fnEnv);
            }
        }

        // Resolve the final function type
        for (auto& pv : paramVars) pv = apply(pv);
        auto finalRet = apply(retVar);
        auto finalType = Ty::fun(paramVars, finalRet ? finalRet : retVar);
        env[fn.name] = finalType;
        fnTypes_[fn.name] = finalType;
        break;
    }
    }
}

// --- run ---

void Infer::run(Program& program) {
    subst_.clear();
    fnTypes_.clear();
    nextVar_ = 0;

    // Pass 1: register all function names with fresh Fun types (for mutual recursion)
    Env env;
    for (const auto& stmt : program.statements) {
        if (stmt->kind == StmtKind::Function) {
            const auto& fn = static_cast<const FunctionStmt&>(*stmt);
            std::vector<TyPtr> paramVars;
            for (std::size_t i = 0; i < fn.params.size(); ++i)
                paramVars.push_back(fresh());
            env[fn.name] = Ty::fun(paramVars, fresh());
        }
    }

    // Pass 2: infer everything
    for (const auto& stmt : program.statements)
        inferStmt(*stmt, env);

    // Pass 3: resolve lingering type variables in fnTypes_ (handles forward references
    // where a function's return type was inferred via a call to a later-defined function
    // whose type was still a fresh variable at the time of inference).
    for (auto& [name, ty] : fnTypes_)
        ty = apply(ty);
}

} // namespace scioh
