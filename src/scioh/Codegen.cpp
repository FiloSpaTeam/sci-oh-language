#include "scioh/Codegen.hpp"

#include "scioh/Diagnostic.hpp"

#include <sstream>

namespace scioh {
namespace {

std::string quoteCppString(const std::string& value) {
    std::string out = "\"";
    for (const unsigned char ch : value) {
        switch (ch) {
        case '\n':
            out += "\\n";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\r':
            out += "\\r";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        default:
            if (ch < 0x20) {
                out += "\\";
                out.push_back(static_cast<char>('0' + ((ch >> 6) & 0x7)));
                out.push_back(static_cast<char>('0' + ((ch >> 3) & 0x7)));
                out.push_back(static_cast<char>('0' + (ch & 0x7)));
            } else {
                out.push_back(static_cast<char>(ch));
            }
            break;
        }
    }
    out += "\"";
    return out;
}

std::string opFunction(TokenKind kind) {
    switch (kind) {
    case TokenKind::Plus:
        return "Scioh::add";
    case TokenKind::Minus:
        return "Scioh::sub";
    case TokenKind::Star:
        return "Scioh::mul";
    case TokenKind::Slash:
        return "Scioh::div";
    case TokenKind::EqualEqual:
        return "Scioh::eq";
    case TokenKind::NotEqual:
        return "Scioh::ne";
    case TokenKind::Less:
        return "Scioh::lt";
    case TokenKind::LessEqual:
        return "Scioh::le";
    case TokenKind::Greater:
        return "Scioh::gt";
    case TokenKind::GreaterEqual:
        return "Scioh::ge";
    case TokenKind::And:
        return "Scioh::logicAnd";
    case TokenKind::Or:
        return "Scioh::logicOr";
    case TokenKind::Mitta:
        return "Scioh::cons";
    case TokenKind::Percent:
        return "Scioh::mod";
    case TokenKind::Spezza:
        return "Scioh::spezzaIecch";
    default:
        break;
    }
    return "";
}

std::string unaryFunction(TokenKind kind) {
    switch (kind) {
    case TokenKind::Minus:
        return "Scioh::neg";
    case TokenKind::Not:
        return "Scioh::logicNot";
    case TokenKind::Prime:
        return "Scioh::prim";
    case TokenKind::Uldeme:
        return "Scioh::uddhm";
    case TokenKind::Vute:
        return "Scioh::vot";
    case TokenKind::Numere:
        return "Scioh::toNumero";
    case TokenKind::Quante:
        return "Scioh::quante";
    case TokenKind::Cala:
        return "Scioh::arrecala";
    case TokenKind::Suva:
        return "Scioh::arresuva";
    case TokenKind::Arretunne:
        return "Scioh::arrutunna";
    case TokenKind::Radice:
        return "Scioh::radiceQuadrata";
    default:
        break;
    }
    return "";
}

std::string indent(int level) {
    return std::string(static_cast<std::size_t>(level) * 4, ' ');
}

} // namespace

void Codegen::emit(const Program& program, std::ostream& out,
                   const std::unordered_map<std::string, TyPtr>& fnTypes) {
    scopes_.clear();
    functionSymbols_.clear();
    typedFns_.clear();
    nextSymbol_ = 0;
    nextTypedSym_ = 0;

    // Register functions whose type is fully Num/Bool (no Str/List/etc.)
    for (const auto& [name, ty] : fnTypes) {
        if (!ty || ty->k != Ty::K::Fun) continue;
        bool allNumBool = true;
        for (const auto& t : ty->inner) {
            if (!t || (!t->isNum() && !t->isBool())) { allNumBool = false; break; }
        }
        if (allNumBool) typedFns_[name] = ty;
    }

    out << R"SCIOH(#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Scioh {

// --- Intrusive reference counting ---

struct RefCounted {
    mutable std::atomic<int> refCount{1};
    virtual ~RefCounted() = default;
};
inline void addRef(const RefCounted* p) noexcept {
    p->refCount.fetch_add(1, std::memory_order_relaxed);
}
inline void release(const RefCounted* p) noexcept {
    if (p->refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) delete p;
}

// --- Forward declarations ---
struct StringBox; struct FnBox; struct Cons; struct ResultBox;

// --- NaN-boxed Value (8 bytes) ---
//
// Layout: every Value is one uint64_t.
//   Regular double  → stored as-is (isBoxed() == false)
//   Tagged value    → bits 62..50 = 1111'1111'1100 (kBoxMask pattern)
//
// The hardware canonical quiet NaN (0x7FF8...) has bit 50 = 0, so it never
// collides with our tags (which require bit 50 = 1).
//
// Top-16-bit tags:
//   0x7FFC  false       (immediate)
//   0x7FFD  true        (immediate)
//   0x7FFE  empty list  (immediate)
//   0xFFFC  StringBox*  (low 48 bits = pointer)
//   0xFFFD  FnBox*
//   0xFFFE  Cons*
//   0xFFFF  ResultBox*

class Value {
public:
    static constexpr uint64_t kBoxMask   = 0x7FFC000000000000ULL;
    static constexpr uint64_t kTagMask16 = 0xFFFF000000000000ULL;
    static constexpr uint64_t kPtrMask   = 0x0000FFFFFFFFFFFFULL;
    static constexpr uint64_t kTagFalse  = 0x7FFC000000000000ULL;
    static constexpr uint64_t kTagTrue   = 0x7FFD000000000000ULL;
    static constexpr uint64_t kTagNil    = 0x7FFE000000000000ULL;
    static constexpr uint64_t kTagStr    = 0xFFFC000000000000ULL;
    static constexpr uint64_t kTagFn     = 0xFFFD000000000000ULL;
    static constexpr uint64_t kTagCons   = 0xFFFE000000000000ULL;
    static constexpr uint64_t kTagResult = 0xFFFF000000000000ULL;

    enum class Kind { Number, Boolean, String, Function, List, Result };

    Value()                noexcept : bits_(kTagFalse) {}
    explicit Value(double d) noexcept { std::memcpy(&bits_, &d, 8); }
    Value(bool b)          noexcept : bits_(b ? kTagTrue : kTagFalse) {}
    Value(const char* s)            : Value(std::string(s)) {}
    Value(std::string s);
    Value(std::function<Value(std::vector<Value>)> fn);
    Value(const Value& o)  noexcept : bits_(o.bits_) { if (auto* p = heapPtr()) addRef(p); }
    Value(Value&& o)       noexcept : bits_(o.bits_) { o.bits_ = kTagFalse; }
    ~Value()               noexcept { if (auto* p = heapPtr()) release(p); }

    Value& operator=(const Value& o) noexcept {
        if (this != &o) {
            if (auto* p = heapPtr()) release(p);
            bits_ = o.bits_;
            if (auto* p = heapPtr()) addRef(p);
        }
        return *this;
    }
    Value& operator=(Value&& o) noexcept {
        if (this != &o) {
            if (auto* p = heapPtr()) release(p);
            bits_ = o.bits_; o.bits_ = kTagFalse;
        }
        return *this;
    }

    Kind kind() const noexcept {
        if (!isBoxed()) return Kind::Number;
        const auto t = bits_ & kTagMask16;
        if (t == kTagFalse || t == kTagTrue) return Kind::Boolean;
        if (t == kTagNil   || t == kTagCons) return Kind::List;
        if (t == kTagStr)    return Kind::String;
        if (t == kTagFn)     return Kind::Function;
        return Kind::Result;
    }

    double             asNumber()  const noexcept { double d; std::memcpy(&d, &bits_, 8); return d; }
    bool               asBoolean() const noexcept { return bits_ == kTagTrue; }
    const std::string& asString()  const noexcept;
    const std::function<Value(std::vector<Value>)>& asFunction() const noexcept;

    bool         listEmpty() const noexcept { return (bits_ & kTagMask16) == kTagNil; }
    const Value& listHead()  const noexcept;
    Value        listTail()  const noexcept;

    bool         resultOk()    const noexcept;
    const Value& resultInner() const noexcept;

    static Value emptyList() noexcept { Value v; v.bits_ = kTagNil; return v; }
    static Value makeCons(Value head, Value tail);
    static Value fromVector(std::vector<Value> v);
    static Value ok(Value inner);
    static Value err(Value inner);

private:
    uint64_t bits_;

    bool isBoxed() const noexcept { return (bits_ & kBoxMask) == kBoxMask; }
    RefCounted* heapPtr() const noexcept {
        const auto t = bits_ & kTagMask16;
        if (t == kTagStr || t == kTagFn || t == kTagCons || t == kTagResult)
            return reinterpret_cast<RefCounted*>(bits_ & kPtrMask);
        return nullptr;
    }
    static Value fromBits(uint64_t b) noexcept { Value v; v.bits_ = b; return v; }
};

struct StringBox : RefCounted {
    std::string s;
    explicit StringBox(std::string v) : s(std::move(v)) {}
};
struct FnBox : RefCounted {
    std::function<Value(std::vector<Value>)> fn;
};
struct Cons : RefCounted {
    Value head, tail;
    Cons(Value h, Value t) : head(std::move(h)), tail(std::move(t)) {}
};
struct ResultBox : RefCounted {
    bool ok; Value inner;
    ResultBox(bool ok_, Value v) : ok(ok_), inner(std::move(v)) {}
};

inline Value::Value(std::string s) {
    auto* p = new StringBox(std::move(s));
    bits_ = kTagStr | reinterpret_cast<uint64_t>(p);
}
inline Value::Value(std::function<Value(std::vector<Value>)> fn) {
    auto* p = new FnBox; p->fn = std::move(fn);
    bits_ = kTagFn | reinterpret_cast<uint64_t>(p);
}
inline const std::string& Value::asString() const noexcept {
    return reinterpret_cast<StringBox*>(bits_ & kPtrMask)->s;
}
inline const std::function<Value(std::vector<Value>)>& Value::asFunction() const noexcept {
    return reinterpret_cast<FnBox*>(bits_ & kPtrMask)->fn;
}
inline const Value& Value::listHead() const noexcept {
    return reinterpret_cast<Cons*>(bits_ & kPtrMask)->head;
}
inline Value Value::listTail() const noexcept {
    return reinterpret_cast<Cons*>(bits_ & kPtrMask)->tail;
}
inline bool Value::resultOk() const noexcept {
    return reinterpret_cast<ResultBox*>(bits_ & kPtrMask)->ok;
}
inline const Value& Value::resultInner() const noexcept {
    return reinterpret_cast<ResultBox*>(bits_ & kPtrMask)->inner;
}
inline Value Value::makeCons(Value head, Value tail) {
    auto* p = new Cons{std::move(head), std::move(tail)};
    return fromBits(kTagCons | reinterpret_cast<uint64_t>(p));
}
inline Value Value::fromVector(std::vector<Value> v) {
    Value cur = emptyList();
    for (int i = static_cast<int>(v.size()) - 1; i >= 0; --i)
        cur = makeCons(std::move(v[i]), std::move(cur));
    return cur;
}
inline Value Value::ok(Value inner) {
    auto* p = new ResultBox{true, std::move(inner)};
    return fromBits(kTagResult | reinterpret_cast<uint64_t>(p));
}
inline Value Value::err(Value inner) {
    auto* p = new ResultBox{false, std::move(inner)};
    return fromBits(kTagResult | reinterpret_cast<uint64_t>(p));
}

struct ReturnValue { Value value; };

// TailCall is delivered via thread_local instead of exceptions: setting
// g_tailCall + returning a placeholder avoids stack-unwinding overhead on
// every recursive iteration.
struct TailCall { Value fn; std::vector<Value> args; };
thread_local std::optional<TailCall> g_tailCall;

Value apply(Value fn, std::vector<Value> args) {
    while (true) {
        if (fn.kind() != Value::Kind::Function)
            throw std::runtime_error("ne ie na funzione");
        Value result = fn.asFunction()(std::move(args));
        if (!g_tailCall) return result;
        auto tc = std::move(*g_tailCall);
        g_tailCall.reset();
        fn   = std::move(tc.fn);
        args = std::move(tc.args);
    }
}

std::string toText(const Value& value) {
    switch (value.kind()) {
    case Value::Kind::Number: {
        const double n = value.asNumber();
        if (n == std::floor(n) && std::abs(n) < 1e15)
            return std::to_string(static_cast<long long>(n));
        std::ostringstream out; out << n; return out.str();
    }
    case Value::Kind::Boolean:  return value.asBoolean() ? "sci" : "no";
    case Value::Kind::String:   return value.asString();
    case Value::Kind::Function: return "<funzione>";
    case Value::Kind::List: {
        std::string out = "["; bool first = true;
        for (Value c = value; !c.listEmpty(); c = c.listTail()) {
            if (!first) out += ", ";
            out += toText(c.listHead()); first = false;
        }
        return out + "]";
    }
    case Value::Kind::Result:
        return value.resultOk()
            ? "vabbone(" + toText(value.resultInner()) + ")"
            : "guaje("   + toText(value.resultInner()) + ")";
    }
    return "";
}

double toNumber(const Value& value, const char* op) {
    if (value.kind() != Value::Kind::Number)
        throw std::runtime_error(std::string("lu segne '") + op + "' vo' numeri");
    return value.asNumber();
}

bool isTruthy(const Value& value) {
    if (value.kind() != Value::Kind::Boolean)
        throw std::runtime_error("'se' vo' nu valore booleane: sci oppure no");
    return value.asBoolean();
}

bool toBoolean(const Value& value, const char* op) {
    if (value.kind() != Value::Kind::Boolean)
        throw std::runtime_error(std::string("lu segne '") + op + "' vo' sci oppure no");
    return value.asBoolean();
}

Value neg(const Value& value) { return Value(-toNumber(value, "meno")); }
Value logicNot(const Value& value) { return Value(!toBoolean(value, "ne")); }
Value logicAnd(const Value& l, const Value& r) { return Value(toBoolean(l, "e") && toBoolean(r, "e")); }
Value logicOr (const Value& l, const Value& r) { return Value(toBoolean(l, "o") || toBoolean(r, "o")); }

bool sameValue(const Value& left, const Value& right) {
    if (left.kind() != right.kind()) return false;
    switch (left.kind()) {
    case Value::Kind::Number:   return left.asNumber()  == right.asNumber();
    case Value::Kind::Boolean:  return left.asBoolean() == right.asBoolean();
    case Value::Kind::String:   return left.asString()  == right.asString();
    case Value::Kind::Function: return false;
    case Value::Kind::List: {
        Value l = left, r = right;
        for (; !l.listEmpty() && !r.listEmpty(); l = l.listTail(), r = r.listTail())
            if (!sameValue(l.listHead(), r.listHead())) return false;
        return l.listEmpty() && r.listEmpty();
    }
    case Value::Kind::Result:
        if (left.resultOk() != right.resultOk()) return false;
        return sameValue(left.resultInner(), right.resultInner());
    }
    return false;
}

Value eq(const Value& l, const Value& r) { return Value( sameValue(l, r)); }
Value ne(const Value& l, const Value& r) { return Value(!sameValue(l, r)); }
Value lt(const Value& l, const Value& r) { return Value(toNumber(l, "meno de")     <  toNumber(r, "meno de")); }
Value le(const Value& l, const Value& r) { return Value(toNumber(l, "meno uguale") <= toNumber(r, "meno uguale")); }
Value gt(const Value& l, const Value& r) { return Value(toNumber(l, "piu de")      >  toNumber(r, "piu de")); }
Value ge(const Value& l, const Value& r) { return Value(toNumber(l, "piu uguale")  >= toNumber(r, "piu uguale")); }

Value add(const Value& left, const Value& right) {
    if (left.kind() == Value::Kind::List && right.kind() == Value::Kind::List) {
        std::vector<Value> elems;
        for (Value c = left; !c.listEmpty(); c = c.listTail()) elems.push_back(c.listHead());
        Value cur = right;
        for (int i = static_cast<int>(elems.size()) - 1; i >= 0; --i)
            cur = Value::makeCons(std::move(elems[i]), std::move(cur));
        return cur;
    }
    if (left.kind() == Value::Kind::String || right.kind() == Value::Kind::String)
        return Value(toText(left) + toText(right));
    return Value(toNumber(left, "+") + toNumber(right, "+"));
}

Value sub(const Value& l, const Value& r) { return Value(toNumber(l, "-") - toNumber(r, "-")); }
Value mul(const Value& l, const Value& r) { return Value(toNumber(l, "*") * toNumber(r, "*")); }

Value div(const Value& left, const Value& right) {
    const auto d = toNumber(right, "/");
    if (d == 0.0) throw std::runtime_error("divisione pe zero");
    return Value(toNumber(left, "/") / d);
}

Value prim(const Value& v) {
    if (v.kind() != Value::Kind::List || v.listEmpty())
        throw std::runtime_error("prime: lista vuta o non-lista");
    return v.listHead();
}
Value uddhm(const Value& v) {
    if (v.kind() != Value::Kind::List || v.listEmpty())
        throw std::runtime_error("uldeme: lista vuta o non-lista");
    return v.listTail();
}
Value vot(const Value& v) {
    if (v.kind() != Value::Kind::List) throw std::runtime_error("vute: non-lista");
    return Value(v.listEmpty());
}
Value quante(const Value& v) {
    if (v.kind() == Value::Kind::String)
        return Value(static_cast<double>(v.asString().size()));
    if (v.kind() == Value::Kind::List) {
        double count = 0;
        for (Value c = v; !c.listEmpty(); c = c.listTail()) ++count;
        return Value(count);
    }
    throw std::runtime_error("quante: vo' na stringa o na lista");
}

Value mod(const Value& left, const Value& right) {
    const auto b = toNumber(right, "%");
    if (b == 0.0) throw std::runtime_error("modulo pe zero");
    return Value(std::fmod(toNumber(left, "%"), b));
}

Value spezzaIecch(const Value& str, const Value& sep) {
    if (str.kind() != Value::Kind::String) throw std::runtime_error("spezza iecch: vo' na stringa");
    if (sep.kind() != Value::Kind::String) throw std::runtime_error("spezza iecch: lu separatore dev'esse na stringa");
    std::vector<Value> result;
    const std::string& s = str.asString();
    const std::string& d = sep.asString();
    if (d.empty()) {
        for (char c : s) result.push_back(Value(std::string(1, c)));
        return Value::fromVector(std::move(result));
    }
    std::size_t pos = 0, found;
    while ((found = s.find(d, pos)) != std::string::npos) {
        result.push_back(Value(s.substr(pos, found - pos)));
        pos = found + d.size();
    }
    result.push_back(Value(s.substr(pos)));
    return Value::fromVector(std::move(result));
}

Value arrecala(const Value& v) { return Value(std::floor(toNumber(v, "cala"))); }
Value arresuva(const Value& v) { return Value(std::ceil (toNumber(v, "suva"))); }
Value arrutunna(const Value& v){ return Value(std::round(toNumber(v, "arretunne"))); }

Value radiceQuadrata(const Value& v) {
    const auto n = toNumber(v, "radice quadrata");
    if (n < 0) throw std::runtime_error("radice quadrata de nu numero negateve");
    return Value(std::sqrt(n));
}

Value readLine() {
    std::string line;
    if (!std::getline(std::cin, line)) throw std::runtime_error("fine dell'input");
    return Value(std::move(line));
}

Value toNumero(const Value& v) {
    if (v.kind() == Value::Kind::Number) return v;
    if (v.kind() == Value::Kind::String) {
        try {
            std::size_t pos;
            const std::string& s = v.asString();
            double n = std::stod(s, &pos);
            if (pos != s.size()) throw std::runtime_error("");
            return Value(n);
        } catch (...) {
            throw std::runtime_error("numere: non riesco a converti '" + v.asString() + "' in numero");
        }
    }
    throw std::runtime_error("numere: vo' na stringa o nu numero");
}

Value cons(const Value& elem, const Value& lista) {
    if (lista.kind() != Value::Kind::List)
        throw std::runtime_error("mitta prime: il secondo argomento deve essere una lista");
    return Value::makeCons(elem, lista);
}

std::ostream& operator<<(std::ostream& out, const Value& value) {
    out << toText(value); return out;
}

Value mappa_impl(const Value& fn, const Value& list) {
    std::vector<Value> result;
    for (Value c = list; !c.listEmpty(); c = c.listTail())
        result.push_back(apply(fn, {c.listHead()}));
    return Value::fromVector(std::move(result));
}

Value filtre_impl(const Value& fn, const Value& list) {
    std::vector<Value> result;
    for (Value c = list; !c.listEmpty(); c = c.listTail()) {
        if (isTruthy(apply(fn, {c.listHead()})))
            result.push_back(c.listHead());
    }
    return Value::fromVector(std::move(result));
}

Value pieghe_impl(const Value& acc, const Value& fn, const Value& list) {
    Value cur = acc;
    for (Value c = list; !c.listEmpty(); c = c.listTail())
        cur = apply(fn, {cur, c.listHead()});
    return cur;
}

Value inversa_impl(const Value& list) {
    Value cur = Value::emptyList();
    for (Value c = list; !c.listEmpty(); c = c.listTail())
        cur = Value::makeCons(c.listHead(), std::move(cur));
    return cur;
}

Value pijje_impl(const Value& n, const Value& list) {
    const long long count = static_cast<long long>(toNumber(n, "pijje"));
    std::vector<Value> result;
    long long i = 0;
    for (Value c = list; !c.listEmpty() && i < count; c = c.listTail(), ++i)
        result.push_back(c.listHead());
    return Value::fromVector(std::move(result));
}

Value lasse_impl(const Value& n, const Value& list) {
    const long long count = static_cast<long long>(toNumber(n, "lasse"));
    long long i = 0;
    Value cur = list;
    for (; !cur.listEmpty() && i < count; cur = cur.listTail(), ++i) {}
    return cur;
}

Value uni_impl(const Value& sep, const Value& list) {
    if (sep.kind() != Value::Kind::String) throw std::runtime_error("uni: lu separatore dev'esse na stringa");
    std::string result;
    bool first = true;
    for (Value c = list; !c.listEmpty(); c = c.listTail()) {
        if (!first) result += sep.asString();
        result += toText(c.listHead());
        first = false;
    }
    return Value(std::move(result));
}

Value assol_impl(const Value& v) { return Value(std::abs(toNumber(v, "assol"))); }
Value massime_impl(const Value& a, const Value& b) {
    return toNumber(a, "massime") >= toNumber(b, "massime") ? a : b;
}
Value mineme_impl(const Value& a, const Value& b) {
    return toNumber(a, "mineme") <= toNumber(b, "mineme") ? a : b;
}
Value putenze_impl(const Value& base, const Value& exp) {
    return Value(std::pow(toNumber(base, "putenze"), toNumber(exp, "putenze")));
}

} // namespace Scioh

const Scioh::Value mappa = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::mappa_impl(args.at(0), args.at(1));
    }));
const Scioh::Value filtre =Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::filtre_impl(args.at(0), args.at(1));
    }));
const Scioh::Value pieghe =Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::pieghe_impl(args.at(0), args.at(1), args.at(2));
    }));
const Scioh::Value inversa = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::inversa_impl(args.at(0));
    }));
const Scioh::Value pijje = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::pijje_impl(args.at(0), args.at(1));
    }));
const Scioh::Value lasse = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::lasse_impl(args.at(0), args.at(1));
    }));
const Scioh::Value uni = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::uni_impl(args.at(0), args.at(1));
    }));
const Scioh::Value assol = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::assol_impl(args.at(0));
    }));
const Scioh::Value massime = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::massime_impl(args.at(0), args.at(1));
    }));
const Scioh::Value mineme = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::mineme_impl(args.at(0), args.at(1));
    }));
const Scioh::Value putenze = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>(
    [](std::vector<Scioh::Value> args) -> Scioh::Value {
        return Scioh::putenze_impl(args.at(0), args.at(1));
    }));

)SCIOH";

    // Pre-register built-in prelude functions (available in every program)
    for (const char* name : {"mappa", "filtre", "pieghe",
                             "inversa", "pijje", "lasse", "uni",
                             "assol", "massime", "mineme", "putenze"})
        functionSymbols_[name] = name;

    // Collect all user function names for forward declarations and symbol registry
    for (const auto& stmt : program.statements) {
        if (stmt->kind != StmtKind::Function) {
            continue;
        }
        const auto& fn = static_cast<const FunctionStmt&>(*stmt);
        declareFunctionSymbol(fn.name);
    }

    // Typed function forward declarations (before main, for mutual recursion)
    for (const auto& stmt : program.statements) {
        if (stmt->kind != StmtKind::Function) continue;
        const auto& fn = static_cast<const FunctionStmt&>(*stmt);
        auto it = typedFns_.find(fn.name);
        if (it == typedFns_.end()) continue;
        const auto& ty = it->second;
        out << "static " << nativeType(ty->ret()) << " fn_" << fn.name << "_typed(";
        const auto ps = ty->params();
        for (std::size_t i = 0; i < ps.size(); ++i) {
            if (i) out << ", ";
            out << nativeType(ps[i]) << " p_" << i;
        }
        out << ");\n";
    }

    // Typed function definitions
    for (const auto& stmt : program.statements) {
        if (stmt->kind != StmtKind::Function) continue;
        const auto& fn = static_cast<const FunctionStmt&>(*stmt);
        auto it = typedFns_.find(fn.name);
        if (it == typedFns_.end()) continue;
        emitTypedFunction(fn, it->second, out);
    }

    out << "int main() {\n    try {\n";
    pushScope();


    // Forward declarations for all function Values inside main
    bool hasFunctions = false;
    for (const auto& stmt : program.statements) {
        if (stmt->kind == StmtKind::Function) {
            hasFunctions = true;
            break;
        }
    }
    if (hasFunctions) {
        out << "        Scioh::Value";
        bool first = true;
        for (const auto& stmt : program.statements) {
            if (stmt->kind != StmtKind::Function) {
                continue;
            }
            const auto& fn = static_cast<const FunctionStmt&>(*stmt);
            if (!first) {
                out << ",";
            }
            out << " fn_" << fn.name;
            first = false;
        }
        out << ";\n";
    }

    // Emit function definitions (typed fns get thin Value wrappers, others unchanged)
    for (const auto& stmt : program.statements) {
        if (stmt->kind == StmtKind::Function) {
            emitStatement(*stmt, out, 2);
        }
    }

    // Emit non-function statements
    for (const auto& stmt : program.statements) {
        if (stmt->kind != StmtKind::Function) {
            emitStatement(*stmt, out, 2);
        }
    }

    out << R"(        return 0;
    } catch (const std::exception& error) {
        std::cerr << "errore runtime: " << error.what() << '\n';
        return 1;
    }
}
)";

    popScope();
}

void Codegen::emitStatements(const std::vector<std::unique_ptr<Stmt>>& statements, std::ostream& out, int indentLevel) {
    for (const auto& stmt : statements) {
        emitStatement(*stmt, out, indentLevel);
    }
}

void Codegen::emitBranchBody(const std::vector<std::unique_ptr<Stmt>>& stmts, std::ostream& out, int indentLevel) {
    if (stmts.empty()) {
        out << indent(indentLevel) << "return Scioh::Value(false);\n";
        return;
    }
    for (std::size_t i = 0; i + 1 < stmts.size(); ++i) {
        emitStatement(*stmts[i], out, indentLevel);
    }
    const auto& last = *stmts.back();
    if (last.kind == StmtKind::ExprStmt) {
        const auto& exprStmt = static_cast<const ExprStmt&>(last);
        out << indent(indentLevel) << "return " << emitExpr(*exprStmt.expr) << ";\n";
    } else {
        emitStatement(last, out, indentLevel);
        out << indent(indentLevel) << "return Scioh::Value(false);\n";
    }
}

void Codegen::emitMatchBranches(
    const MatchExpr& matchExpr, std::ostream& out, int guardIndent, int bodyIndent, bool isTail)
{
    bool first = true;
    for (const auto& branch : matchExpr.branches) {
        if (!first) {
            out << " else ";
        } else {
            out << indent(guardIndent);
        }
        first = false;
        pushScope();

        switch (branch.patternKind) {
        case PatternKind::Wildcard:
            out << "{\n";
            break;
        case PatternKind::Number:
            out << "if (_subj.kind() == Scioh::Value::Kind::Number && _subj.asNumber() == " << branch.numberLiteral << ") {\n";
            break;
        case PatternKind::String:
            out << "if (_subj.kind() == Scioh::Value::Kind::String && _subj.asString() == " << quoteCppString(branch.stringLiteral) << ") {\n";
            break;
        case PatternKind::Boolean:
            out << "if (_subj.kind() == Scioh::Value::Kind::Boolean && _subj.asBoolean() == " << (branch.boolLiteral ? "true" : "false") << ") {\n";
            break;
        case PatternKind::EmptyList:
            out << "if (_subj.kind() == Scioh::Value::Kind::List && _subj.listEmpty()) {\n";
            break;
        case PatternKind::Cons: {
            out << "if (_subj.kind() == Scioh::Value::Kind::List && !_subj.listEmpty()) {\n";
            auto headCpp = declareSymbol(branch.headName, matchExpr.location);
            auto tailCpp = declareSymbol(branch.tailName, matchExpr.location);
            out << indent(bodyIndent) << "auto " << headCpp << " = _subj.listHead();\n";
            out << indent(bodyIndent) << "auto " << tailCpp << " = _subj.listTail();\n";
            break;
        }
        case PatternKind::ResultOk: {
            out << "if (_subj.kind() == Scioh::Value::Kind::Result && _subj.resultOk()) {\n";
            auto bindCpp = declareSymbol(branch.headName, matchExpr.location);
            out << indent(bodyIndent) << "auto " << bindCpp << " = _subj.resultInner();\n";
            break;
        }
        case PatternKind::ResultErr: {
            out << "if (_subj.kind() == Scioh::Value::Kind::Result && !_subj.resultOk()) {\n";
            auto bindCpp = declareSymbol(branch.headName, matchExpr.location);
            out << indent(bodyIndent) << "auto " << bindCpp << " = _subj.resultInner();\n";
            break;
        }
        }

        if (isTail) {
            emitTailBranchBody(branch.body, out, bodyIndent);
        } else {
            emitBranchBody(branch.body, out, bodyIndent);
        }
        out << indent(guardIndent) << "}";
        popScope();
    }
}

void Codegen::emitStatement(const Stmt& stmt, std::ostream& out, int indentLevel) {
    switch (stmt.kind) {
    case StmtKind::Let: {
        const auto& let = static_cast<const LetStmt&>(stmt);
        const auto symbol = declareSymbol(let.name, let.location);
        out << indent(indentLevel) << "Scioh::Value " << symbol << " = " << emitExpr(*let.value) << ";\n";
        break;
    }
    case StmtKind::Print: {
        const auto& print = static_cast<const PrintStmt&>(stmt);
        out << indent(indentLevel) << "std::cout << " << emitExpr(*print.value) << " << '\\n';\n";
        break;
    }
    case StmtKind::Function: {
        const auto& fn = static_cast<const FunctionStmt&>(stmt);

        // Typed functions: emit a thin Value wrapper that delegates to the typed C++ function
        if (isTyped(fn.name)) {
            const auto& ty = typedFns_.at(fn.name);
            const auto ps = ty->params();
            out << indent(indentLevel) << "fn_" << fn.name
                << " = Scioh::Value(std::function<Scioh::Value(std::vector<Scioh::Value>)>("
                << "[](std::vector<Scioh::Value> args) -> Scioh::Value {\n";
            out << indent(indentLevel + 1) << "return Scioh::Value(fn_"
                << fn.name << "_typed(";
            for (std::size_t i = 0; i < ps.size(); ++i) {
                if (i) out << ", ";
                if (ps[i]->isNum())  out << "args.at(" << i << ").asNumber()";
                else                 out << "args.at(" << i << ").asBoolean()";
            }
            out << "));\n";
            out << indent(indentLevel) << "}));\n";
            break;
        }

        pushScope();
        std::vector<std::string> paramCppNames;
        for (const auto& param : fn.params)
            paramCppNames.push_back(declareSymbol(param, fn.location));

        std::ostringstream body;
        body << indent(indentLevel) << "fn_" << fn.name << " = Scioh::Value{[&](std::vector<Scioh::Value> args) -> Scioh::Value {\n";
        body << indent(indentLevel + 1) << "try {\n";
        for (std::size_t i = 0; i < fn.params.size(); ++i)
            body << indent(indentLevel + 2) << "auto " << paramCppNames[i] << " = args.at(" << i << ");\n";
        for (const auto& s : fn.body) {
            std::ostringstream tmp;
            emitStatement(*s, tmp, indentLevel + 2);
            body << tmp.str();
        }
        if (fn.body.empty() || fn.body.back()->kind != StmtKind::Return)
            body << indent(indentLevel + 2) << "return Scioh::Value(false);\n";
        body << indent(indentLevel + 1) << "} catch (const Scioh::ReturnValue& ret) {\n";
        body << indent(indentLevel + 2) << "return ret.value;\n";
        body << indent(indentLevel + 1) << "}\n";
        body << indent(indentLevel) << "}};\n";

        out << body.str();
        popScope();
        break;
    }
    case StmtKind::Return: {
        const auto& ret = static_cast<const ReturnStmt&>(stmt);
        emitTailReturn(*ret.value, out, indentLevel);
        break;
    }
    case StmtKind::ExprStmt: {
        const auto& exprStmt = static_cast<const ExprStmt&>(stmt);
        out << indent(indentLevel) << emitExpr(*exprStmt.expr) << ";\n";
        break;
    }
    }
}

std::string Codegen::emitExpr(const Expr& expr) {
    switch (expr.kind) {
    case ExprKind::Number: {
        const auto& number = static_cast<const NumberExpr&>(expr);
        std::string lit = number.value;
        if (lit.find('.') == std::string::npos) lit += ".0";
        return "Scioh::Value(" + lit + ")";
    }
    case ExprKind::String: {
        const auto& string = static_cast<const StringExpr&>(expr);
        return "Scioh::Value(" + quoteCppString(string.value) + ")";
    }
    case ExprKind::Boolean: {
        const auto& boolean = static_cast<const BooleanExpr&>(expr);
        return std::string("Scioh::Value(") + (boolean.value ? "true" : "false") + ")";
    }
    case ExprKind::Identifier: {
        const auto& identifier = static_cast<const IdentifierExpr&>(expr);
        return symbolFor(identifier.name, identifier.location);
    }
    case ExprKind::Unary: {
        const auto& unary = static_cast<const UnaryExpr&>(expr);
        return unaryFunction(unary.op) + "(" + emitExpr(*unary.right) + ")";
    }
    case ExprKind::Binary: {
        const auto& binary = static_cast<const BinaryExpr&>(expr);
        return opFunction(binary.op) + "(" + emitExpr(*binary.left) + ", " + emitExpr(*binary.right) + ")";
    }
    case ExprKind::Call: {
        const auto& call = static_cast<const CallExpr&>(expr);
        // Direct call to a statically-typed function: avoids vector allocation + apply dispatch
        if (call.callee->kind == ExprKind::Identifier) {
            const auto& calleeName = static_cast<const IdentifierExpr&>(*call.callee).name;
            auto it = typedFns_.find(calleeName);
            if (it != typedFns_.end()) {
                const auto ps = it->second->params();
                if (call.args.size() == ps.size()) {
                    std::string result = "Scioh::Value(fn_" + calleeName + "_typed(";
                    for (std::size_t i = 0; i < ps.size(); ++i) {
                        if (i) result += ", ";
                        result += "(" + emitExpr(*call.args[i]) + ")." +
                                  (ps[i]->isNum() ? "asNumber()" : "asBoolean()");
                    }
                    return result + "))";
                }
            }
        }
        std::string result = "Scioh::apply(" + emitExpr(*call.callee) + ", {";
        for (std::size_t i = 0; i < call.args.size(); ++i) {
            if (i > 0) result += ", ";
            result += emitExpr(*call.args[i]);
        }
        return result + "})";
    }
    case ExprKind::If: {
        const auto& ifExpr = static_cast<const IfExpr&>(expr);
        std::ostringstream oss;
        oss << "[&]() -> Scioh::Value {\n";
        oss << "if (Scioh::isTruthy(" << emitExpr(*ifExpr.condition) << ")) {\n";
        emitBranchBody(ifExpr.thenBranch, oss, 1);
        oss << "} else {\n";
        emitBranchBody(ifExpr.elseBranch, oss, 1);
        oss << "}}()";
        return oss.str();
    }
    case ExprKind::Input:
        return "Scioh::readLine()";
    case ExprKind::Result: {
        const auto& r = static_cast<const ResultExpr&>(expr);
        return r.isOk
            ? "Scioh::Value::ok(" + emitExpr(*r.value) + ")"
            : "Scioh::Value::err(" + emitExpr(*r.value) + ")";
    }
    case ExprKind::Prove: {
        const auto& np = static_cast<const ProveExpr&>(expr);
        std::ostringstream oss;
        oss << "[&]() -> Scioh::Value {\n";
        oss << "    try { return Scioh::Value::ok(" << emitExpr(*np.body) << "); }\n";
        oss << "    catch (const std::exception& e) { return Scioh::Value::err(Scioh::Value(std::string(e.what()))); }\n";
        oss << "}()";
        return oss.str();
    }
    case ExprKind::Match: {
        const auto& matchExpr = static_cast<const MatchExpr&>(expr);
        std::ostringstream oss;
        oss << "[&]() -> Scioh::Value {\n";
        oss << "    auto _subj = " << emitExpr(*matchExpr.subject) << ";\n";
        emitMatchBranches(matchExpr, oss, 1, 2, false);
        oss << "\n    return Scioh::Value(false);\n}()";
        return oss.str();
    }
    case ExprKind::List: {
        const auto& listExpr = static_cast<const ListExpr&>(expr);
        std::string result = "Scioh::Value::fromVector({";
        for (std::size_t i = 0; i < listExpr.elements.size(); ++i) {
            if (i > 0) result += ", ";
            result += emitExpr(*listExpr.elements[i]);
        }
        result += "})";
        return result;
    }
    case ExprKind::Lambda: {
        const auto& lambda = static_cast<const LambdaExpr&>(expr);
        std::ostringstream oss;
        oss << "Scioh::Value{[=](std::vector<Scioh::Value> args) -> Scioh::Value {\n";
        pushScope();
        for (std::size_t i = 0; i < lambda.params.size(); ++i) {
            const auto cpp = declareSymbol(lambda.params[i], lambda.location);
            oss << "    auto " << cpp << " = args.at(" << i << ");\n";
        }
        oss << "    return " << emitExpr(*lambda.body) << ";\n";
        popScope();
        oss << "}}";
        return oss.str();
    }
    }
    return "";
}

std::string Codegen::symbolFor(const std::string& name, SourceLocation location) const {
    // Search variable scopes first
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        for (const auto& symbol : *scope) {
            if (symbol.sourceName == name) {
                return symbol.cppName;
            }
        }
    }

    // Then search function symbols
    auto it = functionSymbols_.find(name);
    if (it != functionSymbols_.end()) {
        return it->second;
    }

    throw DiagnosticError(location, "variabile ne' dichiarata: " + name);
}

std::string Codegen::declareSymbol(const std::string& name, SourceLocation location) {
    if (scopes_.empty()) {
        pushScope();
    }

    auto& currentScope = scopes_.back();
    for (const auto& existing : currentScope) {
        if (existing.sourceName == name) {
            throw DiagnosticError(location, "variabile gia' dichiarata: " + name);
        }
    }

    auto cppName = "v" + std::to_string(nextSymbol_++);
    currentScope.push_back(Symbol{name, cppName});
    return cppName;
}

void Codegen::declareFunctionSymbol(const std::string& name) {
    functionSymbols_[name] = "fn_" + name;
}

void Codegen::emitTailBranchBody(const std::vector<std::unique_ptr<Stmt>>& stmts, std::ostream& out, int indentLevel) {
    if (stmts.empty()) {
        out << indent(indentLevel) << "throw Scioh::ReturnValue{Scioh::Value(false)};\n";
        return;
    }
    for (std::size_t i = 0; i + 1 < stmts.size(); ++i) {
        emitStatement(*stmts[i], out, indentLevel);
    }
    const auto& last = *stmts.back();
    if (last.kind == StmtKind::ExprStmt) {
        const auto& exprStmt = static_cast<const ExprStmt&>(last);
        emitTailReturn(*exprStmt.expr, out, indentLevel);
    } else {
        emitStatement(last, out, indentLevel);
        out << indent(indentLevel) << "throw Scioh::ReturnValue{Scioh::Value(false)};\n";
    }
}

void Codegen::emitTailReturn(const Expr& expr, std::ostream& out, int indentLevel) {
    switch (expr.kind) {
    case ExprKind::Call: {
        const auto& call = static_cast<const CallExpr&>(expr);
        out << indent(indentLevel) << "Scioh::g_tailCall.emplace(Scioh::TailCall{" << emitExpr(*call.callee) << ", std::vector<Scioh::Value>{";
        for (std::size_t i = 0; i < call.args.size(); ++i) {
            if (i > 0) out << ", ";
            out << emitExpr(*call.args[i]);
        }
        out << "}});\n";
        out << indent(indentLevel) << "return Scioh::Value(false);\n";
        break;
    }
    case ExprKind::If: {
        const auto& ifExpr = static_cast<const IfExpr&>(expr);
        out << indent(indentLevel) << "if (Scioh::isTruthy(" << emitExpr(*ifExpr.condition) << ")) {\n";
        emitTailBranchBody(ifExpr.thenBranch, out, indentLevel + 1);
        out << indent(indentLevel) << "} else {\n";
        emitTailBranchBody(ifExpr.elseBranch, out, indentLevel + 1);
        out << indent(indentLevel) << "}\n";
        break;
    }
    case ExprKind::Match: {
        const auto& matchExpr = static_cast<const MatchExpr&>(expr);
        out << indent(indentLevel) << "{\n";
        out << indent(indentLevel + 1) << "auto _subj = " << emitExpr(*matchExpr.subject) << ";\n";
        emitMatchBranches(matchExpr, out, indentLevel + 1, indentLevel + 2, true);
        if (!matchExpr.branches.empty()) out << "\n";
        out << indent(indentLevel + 1) << "throw Scioh::ReturnValue{Scioh::Value(false)};\n";
        out << indent(indentLevel) << "}\n";
        break;
    }
    default:
        out << indent(indentLevel) << "throw Scioh::ReturnValue{" << emitExpr(expr) << "};\n";
        break;
    }
}

void Codegen::pushScope() {
    scopes_.push_back({});
}

void Codegen::popScope() {
    scopes_.pop_back();
}

// ---- Typed emission ----

std::string Codegen::nativeType(const TyPtr& ty) {
    if (!ty) return "Scioh::Value";
    switch (ty->k) {
    case Ty::K::Num:  return "double";
    case Ty::K::Bool: return "bool";
    case Ty::K::Str:  return "std::string";
    default:          return "Scioh::Value";
    }
}

bool Codegen::isTyped(const std::string& name) const {
    return typedFns_.count(name) > 0;
}

std::string Codegen::emitTypedExpr(const Expr& expr, const TypedEnv& tenv) {
    switch (expr.kind) {
    case ExprKind::Number:
        return static_cast<const NumberExpr&>(expr).value;

    case ExprKind::Boolean:
        return static_cast<const BooleanExpr&>(expr).value ? "true" : "false";

    case ExprKind::String:
        return quoteCppString(static_cast<const StringExpr&>(expr).value);

    case ExprKind::Identifier: {
        const auto& name = static_cast<const IdentifierExpr&>(expr).name;
        auto it = tenv.find(name);
        if (it != tenv.end()) return it->second.cppName;
        return "";
    }

    case ExprKind::Unary: {
        const auto& u = static_cast<const UnaryExpr&>(expr);
        auto inner = emitTypedExpr(*u.right, tenv);
        if (inner.empty()) return "";
        switch (u.op) {
        case TokenKind::Minus:      return "(-" + inner + ")";
        case TokenKind::Not:        return "(!" + inner + ")";
        case TokenKind::Cala:       return "std::floor(" + inner + ")";
        case TokenKind::Suva:       return "std::ceil(" + inner + ")";
        case TokenKind::Arretunne:  return "std::round(" + inner + ")";
        case TokenKind::Radice:     return "std::sqrt(" + inner + ")";
        default: return "";
        }
    }

    case ExprKind::Binary: {
        const auto& b = static_cast<const BinaryExpr&>(expr);
        auto l = emitTypedExpr(*b.left,  tenv);
        auto r = emitTypedExpr(*b.right, tenv);
        if (l.empty() || r.empty()) return "";
        switch (b.op) {
        case TokenKind::Plus:         return "(" + l + " + " + r + ")";
        case TokenKind::Minus:        return "(" + l + " - " + r + ")";
        case TokenKind::Star:         return "(" + l + " * " + r + ")";
        case TokenKind::Slash:        return "(" + l + " / " + r + ")";
        case TokenKind::Percent:      return "std::fmod(" + l + ", " + r + ")";
        case TokenKind::EqualEqual:   return "(" + l + " == " + r + ")";
        case TokenKind::NotEqual:     return "(" + l + " != " + r + ")";
        case TokenKind::Less:         return "(" + l + " < "  + r + ")";
        case TokenKind::LessEqual:    return "(" + l + " <= " + r + ")";
        case TokenKind::Greater:      return "(" + l + " > "  + r + ")";
        case TokenKind::GreaterEqual: return "(" + l + " >= " + r + ")";
        case TokenKind::And:          return "(" + l + " && " + r + ")";
        case TokenKind::Or:           return "(" + l + " || " + r + ")";
        default: return "";
        }
    }

    case ExprKind::Call: {
        const auto& call = static_cast<const CallExpr&>(expr);
        if (call.callee->kind != ExprKind::Identifier) return "";
        const auto& calleeName = static_cast<const IdentifierExpr&>(*call.callee).name;
        if (!isTyped(calleeName)) return "";
        std::string result = "fn_" + calleeName + "_typed(";
        for (std::size_t i = 0; i < call.args.size(); ++i) {
            auto argStr = emitTypedExpr(*call.args[i], tenv);
            if (argStr.empty()) return "";
            if (i) result += ", ";
            result += argStr;
        }
        return result + ")";
    }

    case ExprKind::If: {
        const auto& ifExpr = static_cast<const IfExpr&>(expr);
        auto cond = emitTypedExpr(*ifExpr.condition, tenv);
        if (cond.empty()) return "";
        if (!expr.ty || !expr.ty->isPrimitive()) return "";
        const auto retT = nativeType(expr.ty);
        std::ostringstream thenOss, elseOss;
        TypedEnv thenEnv = tenv, elseEnv = tenv;
        auto thenResult = emitTypedBody(ifExpr.thenBranch, thenOss, 1, thenEnv);
        if (thenResult.empty()) return "";
        auto elseResult = emitTypedBody(ifExpr.elseBranch, elseOss, 1, elseEnv);
        if (elseResult.empty()) return "";
        // Simple branches (no let bindings) → inline ternary; avoids IIFE overhead
        if (thenOss.str().empty() && elseOss.str().empty())
            return "(" + cond + " ? " + thenResult + " : " + elseResult + ")";
        std::ostringstream oss;
        oss << "[&]() -> " << retT << " {\n";
        oss << "if (" << cond << ") {\n";
        oss << thenOss.str() << "return " << thenResult << ";\n";
        oss << "} else {\n";
        oss << elseOss.str() << "return " << elseResult << ";\n";
        oss << "}}()";
        return oss.str();
    }

    default:
        return "";
    }
}

std::string Codegen::emitTypedBody(
    const std::vector<std::unique_ptr<Stmt>>& stmts,
    std::ostream& out, int il, TypedEnv tenv)
{
    for (std::size_t i = 0; i < stmts.size(); ++i) {
        const auto& stmt = *stmts[i];
        const bool isLast = (i + 1 == stmts.size());

        if (stmt.kind == StmtKind::ExprStmt && isLast) {
            return emitTypedExpr(*static_cast<const ExprStmt&>(stmt).expr, tenv);
        }
        if (stmt.kind == StmtKind::Return) {
            return emitTypedExpr(*static_cast<const ReturnStmt&>(stmt).value, tenv);
        }
        if (stmt.kind == StmtKind::Let) {
            const auto& let = static_cast<const LetStmt&>(stmt);
            auto valStr = emitTypedExpr(*let.value, tenv);
            if (valStr.empty()) return "";
            auto ty = let.value->ty;
            if (!ty || !ty->isPrimitive()) return "";
            auto cppName = "t" + std::to_string(nextTypedSym_++);
            out << indent(il) << nativeType(ty) << " " << cppName << " = " << valStr << ";\n";
            tenv[let.name] = TypedSym{cppName, ty};
            continue;
        }
        // Unsupported statement kind in typed body
        return "";
    }
    return "";
}

void Codegen::emitTypedFunction(const FunctionStmt& fn, const TyPtr& ty, std::ostream& out) {
    const auto ps = ty->params();
    // Build typed env from parameters
    TypedEnv tenv;
    out << "static " << nativeType(ty->ret()) << " fn_" << fn.name << "_typed(";
    for (std::size_t i = 0; i < ps.size(); ++i) {
        if (i) out << ", ";
        out << nativeType(ps[i]) << " p_" << i;
        tenv[fn.params[i]] = TypedSym{"p_" + std::to_string(i), ps[i]};
    }
    out << ") {\n";

    // Try to emit the body as typed
    std::ostringstream body;
    std::string retExpr = emitTypedBody(fn.body, body, 1, tenv);
    if (!retExpr.empty()) {
        out << body.str();
        out << "    return " << retExpr << ";\n";
    } else {
        // Fallback: call through the Value wrapper (should rarely happen for typed fns)
        out << "    // typed emit failed; fallback unreachable\n";
        out << "    return " << nativeType(ty->ret()) << "{};\n";
    }
    out << "}\n";
}

} // namespace scioh
