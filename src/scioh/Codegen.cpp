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

void Codegen::emit(const Program& program, std::ostream& out) {
    scopes_.clear();
    functionSymbols_.clear();
    nextSymbol_ = 0;

    out << R"SCIOH(#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Scioh {

struct Cons;
using List = std::shared_ptr<const Cons>;

// Value is a tagged union over all sci-oh runtime types.
// Lists use persistent cons-cells: cons/head/tail are all O(1) with structural sharing.
struct Value {
    struct FnBox;

    std::variant<
        double,                                       // 0 Number
        bool,                                         // 1 Boolean
        std::string,                                  // 2 String
        std::shared_ptr<FnBox>,                       // 3 Function
        List,                                         // 4 List (persistent cons-cell)
        std::pair<bool, std::shared_ptr<Value>>       // 5 Result {ok, inner}
    > data;

    enum class Kind { Number = 0, Boolean = 1, String = 2, Function = 3, List = 4, Result = 5 };
    Kind kind() const noexcept { return static_cast<Kind>(data.index()); }

    struct FnBox { std::function<Value(std::vector<Value>)> fn; };

    Value() : data(false) {}
    Value(double n) : data(n) {}
    Value(bool b) : data(b) {}
    Value(const char* s) : data(std::string(s)) {}
    Value(std::string s) : data(std::move(s)) {}
    Value(std::function<Value(std::vector<Value>)> fn)
        : data(std::make_shared<FnBox>(FnBox{std::move(fn)})) {}
    explicit Value(List l) : data(std::move(l)) {}

    double                   asNumber()   const { return std::get<double>(data); }
    bool                     asBoolean()  const { return std::get<bool>(data); }
    const std::string&       asString()   const { return std::get<std::string>(data); }
    const std::function<Value(std::vector<Value>)>& asFunction() const {
        return std::get<std::shared_ptr<FnBox>>(data)->fn;
    }
    const List& asList() const { return std::get<List>(data); }

    bool         listEmpty() const { return std::get<List>(data) == nullptr; }
    const Value& listHead()  const;
    Value        listTail()  const;

    bool resultOk()            const { return std::get<std::pair<bool, std::shared_ptr<Value>>>(data).first; }
    const Value& resultInner() const { return *std::get<std::pair<bool, std::shared_ptr<Value>>>(data).second; }

    static Value emptyList() { return Value(List{nullptr}); }
    static Value fromVector(std::vector<Value> v);

    static Value ok(Value inner) {
        Value v;
        v.data = std::pair<bool, std::shared_ptr<Value>>{
            true, std::make_shared<Value>(std::move(inner))};
        return v;
    }
    static Value err(Value inner) {
        Value v;
        v.data = std::pair<bool, std::shared_ptr<Value>>{
            false, std::make_shared<Value>(std::move(inner))};
        return v;
    }
};

struct Cons { Value head; List tail; };

inline const Value& Value::listHead() const { return std::get<List>(data)->head; }
inline Value        Value::listTail() const { return Value(std::get<List>(data)->tail); }
inline Value        Value::fromVector(std::vector<Value> v) {
    List cur = nullptr;
    for (int i = static_cast<int>(v.size()) - 1; i >= 0; --i)
        cur = std::make_shared<Cons>(Cons{std::move(v[i]), std::move(cur)});
    return Value(std::move(cur));
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
        std::ostringstream out;
        out << n;
        return out.str();
    }
    case Value::Kind::Boolean:
        return value.asBoolean() ? "sci" : "no";
    case Value::Kind::String:
        return value.asString();
    case Value::Kind::Function:
        return "<funzione>";
    case Value::Kind::List: {
        std::string out = "[";
        bool first = true;
        for (const Cons* c = value.asList().get(); c; c = c->tail.get()) {
            if (!first) out += ", ";
            out += toText(c->head);
            first = false;
        }
        out += "]";
        return out;
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

Value logicAnd(const Value& left, const Value& right) {
    return Value(toBoolean(left, "e") && toBoolean(right, "e"));
}

Value logicOr(const Value& left, const Value& right) {
    return Value(toBoolean(left, "o") || toBoolean(right, "o"));
}

bool sameValue(const Value& left, const Value& right) {
    if (left.kind() != right.kind()) return false;
    switch (left.kind()) {
    case Value::Kind::Number:   return left.asNumber()  == right.asNumber();
    case Value::Kind::Boolean:  return left.asBoolean() == right.asBoolean();
    case Value::Kind::String:   return left.asString()  == right.asString();
    case Value::Kind::Function: return false;
    case Value::Kind::List: {
        const Cons* l = left.asList().get();
        const Cons* r = right.asList().get();
        for (; l && r; l = l->tail.get(), r = r->tail.get())
            if (!sameValue(l->head, r->head)) return false;
        return !l && !r;
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
        // Collect left's nodes, then prepend them to right (sharing right's tail)
        std::vector<const Cons*> leftNodes;
        for (const Cons* c = left.asList().get(); c; c = c->tail.get())
            leftNodes.push_back(c);
        List cur = right.asList();
        for (int i = static_cast<int>(leftNodes.size()) - 1; i >= 0; --i)
            cur = std::make_shared<Cons>(Cons{leftNodes[i]->head, std::move(cur)});
        return Value(std::move(cur));
    }
    if (left.kind() == Value::Kind::String || right.kind() == Value::Kind::String)
        return Value(toText(left) + toText(right));
    return Value(toNumber(left, "+") + toNumber(right, "+"));
}

Value sub(const Value& l, const Value& r) { return Value(toNumber(l, "-") - toNumber(r, "-")); }
Value mul(const Value& l, const Value& r) { return Value(toNumber(l, "*") * toNumber(r, "*")); }

Value div(const Value& left, const Value& right) {
    const auto divisor = toNumber(right, "/");
    if (divisor == 0.0) throw std::runtime_error("divisione pe zero");
    return Value(toNumber(left, "/") / divisor);
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
        for (const Cons* c = v.asList().get(); c; c = c->tail.get()) ++count;
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
    if (str.kind() != Value::Kind::String)
        throw std::runtime_error("spezza iecch: vo' na stringa");
    if (sep.kind() != Value::Kind::String)
        throw std::runtime_error("spezza iecch: lu separatore dev'esse na stringa");
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
    return Value(std::make_shared<Cons>(Cons{elem, lista.asList()}));
}

std::ostream& operator<<(std::ostream& out, const Value& value) {
    out << toText(value);
    return out;
}

} // namespace Scioh

)SCIOH";

    // Collect all function names for forward declarations and symbol registry
    for (const auto& stmt : program.statements) {
        if (stmt->kind != StmtKind::Function) {
            continue;
        }
        const auto& fn = static_cast<const FunctionStmt&>(*stmt);
        declareFunctionSymbol(fn.name);
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

    // Emit function definitions
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
        pushScope();
        // Declare parameters as local symbols in this scope
        std::vector<std::string> paramCppNames;
        for (const auto& param : fn.params) {
            paramCppNames.push_back(declareSymbol(param, fn.location));
        }

        // Build the lambda body
        std::ostringstream body;
        body << indent(indentLevel) << "fn_" << fn.name << " = Scioh::Value{[&](std::vector<Scioh::Value> args) -> Scioh::Value {\n";
        body << indent(indentLevel + 1) << "try {\n";
        for (std::size_t i = 0; i < fn.params.size(); ++i) {
            body << indent(indentLevel + 2) << "auto " << paramCppNames[i] << " = args.at(" << i << ");\n";
        }
        // Emit body statements
        for (const auto& s : fn.body) {
            std::ostringstream tmp;
            emitStatement(*s, tmp, indentLevel + 2);
            body << tmp.str();
        }
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
        return "Scioh::Value(static_cast<double>(" + number.value + "))";
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
        std::string result = "Scioh::apply(" + emitExpr(*call.callee) + ", {";
        for (std::size_t i = 0; i < call.args.size(); ++i) {
            if (i > 0) {
                result += ", ";
            }
            result += emitExpr(*call.args[i]);
        }
        result += "})";
        return result;
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

} // namespace scioh
