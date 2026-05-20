#pragma once

#include <memory>
#include <string>
#include <vector>

namespace scioh {

struct Ty;
using TyPtr = std::shared_ptr<Ty>;

struct Ty {
    enum class K { Num, Bool, Str, List, Fun, Result, Var } k;

    int var = 0;               // K::Var: unique id
    std::vector<TyPtr> inner;  // K::List/Result: [elem]; K::Fun: [p0..pN, ret]

    static TyPtr num()   { return std::make_shared<Ty>(Ty{K::Num}); }
    static TyPtr bool_() { return std::make_shared<Ty>(Ty{K::Bool}); }
    static TyPtr str()   { return std::make_shared<Ty>(Ty{K::Str}); }

    static TyPtr list(TyPtr e) {
        auto t = std::make_shared<Ty>(Ty{K::List});
        t->inner.push_back(std::move(e));
        return t;
    }
    static TyPtr result(TyPtr e) {
        auto t = std::make_shared<Ty>(Ty{K::Result});
        t->inner.push_back(std::move(e));
        return t;
    }
    static TyPtr fun(std::vector<TyPtr> params, TyPtr ret) {
        auto t = std::make_shared<Ty>(Ty{K::Fun});
        t->inner = std::move(params);
        t->inner.push_back(std::move(ret));
        return t;
    }
    static TyPtr freshVar();  // defined in Infer.cpp

    bool isNum()  const { return k == K::Num; }
    bool isBool() const { return k == K::Bool; }
    bool isStr()  const { return k == K::Str; }
    bool isPrimitive() const { return k == K::Num || k == K::Bool || k == K::Str; }

    // For K::Fun: return type is last element of inner
    TyPtr ret() const { return inner.back(); }
    // For K::Fun: parameter types are all but last
    std::vector<TyPtr> params() const {
        return std::vector<TyPtr>(inner.begin(), inner.end() - 1);
    }
};

std::string tyName(const TyPtr& ty);

} // namespace scioh
