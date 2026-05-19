# AGENTS.md — sci-oh

## What this project is

`sci-oh` is a purely functional compiled language based on the Sambenedettese Italian dialect
(San Benedetto del Tronto, Marche region). The compiler is written in C++20 and translates
`.sci` source files to C++, which it then compiles via the system C++ compiler.

## How to build

`cmake` and `ninja` are not available in the container. Use the direct compiler command:

```sh
cd /workspace/build
/usr/bin/c++ -std=c++20 -I../src -o sci-oh \
  ../src/main.cpp \
  ../src/scioh/Lexer.cpp \
  ../src/scioh/Parser.cpp \
  ../src/scioh/Codegen.cpp
```

Always rebuild after making changes.

## How to test

```sh
# Run the functional example (primary test)
./build/sci-oh examples/funzionale.sci -o /tmp/test_fn && /tmp/test_fn

# Run the basic example
./build/sci-oh examples/ciao.sci -o /tmp/test_ciao && /tmp/test_ciao

# Inspect generated C++ (useful for debugging code generation)
./build/sci-oh --emit-cpp examples/funzionale.sci

# Verify immutability error
echo 'mitt x 5
x vale 10' | ./build/sci-oh /dev/stdin
# Expected: error containing "immutabili"
```

## File map

| File | Purpose |
|---|---|
| `src/scioh/Token.hpp` | `TokenKind` enum, `Token` struct |
| `src/scioh/Lexer.hpp/.cpp` | Tokenizer. All dialect keywords are in `keywordKind()` |
| `src/scioh/Ast.hpp` | All AST node types (Expr and Stmt hierarchies) |
| `src/scioh/Parser.hpp/.cpp` | Recursive-descent parser |
| `src/scioh/Codegen.hpp/.cpp` | C++ code generator |
| `src/scioh/Diagnostic.hpp` | `DiagnosticError` exception with source location |
| `src/scioh/SourceLocation.hpp` | `{line, column}` struct |
| `src/main.cpp` | CLI argument parsing, file I/O, C++ compiler invocation |
| `examples/funzionale.sci` | Primary example: functions, first-class values, recursion, `ie` as expression |
| `examples/ciao.sci` | Basic example: bindings, conditionals, recursion |
| `examples/lambda.sci` | Lambda expressions: closures, higher-order functions, `mbe`/`fine` syntax |

## Compiler pipeline

```
.sci source
  └─ Lexer        → std::vector<Token>
  └─ Parser       → Program (owns vector<unique_ptr<Stmt>>)
  └─ Codegen      → std::string (C++ source)
  └─ system c++   → binary
```

## Language paradigm — PURELY FUNCTIONAL

These are **architectural constraints**, not negotiable:

1. **Immutable bindings**: `mitt x 5` — the only way to bind a name. Reassignment (`x vale 10`) is a compile-time error. Do not add mutable assignment.
2. **No loops**: `mentre` (while) is removed from the language. Use recursion. Do not add it back.
3. **`ie` is an expression**: `altrimenti` is mandatory. `ie` evaluates to the value of the selected branch. It can appear anywhere an expression is valid.
4. **First-class functions**: a function name without `()` evaluates to a `Scioh::Value` wrapping a `std::function`. Passing functions as arguments works via `Scioh::apply()`.

## Current keyword table

All keywords live in `Lexer.cpp` → `keywordKind()`. To add a dialect synonym, add a line there only.

| Dialect word(s) | Token | Meaning |
|---|---|---|
| `mitt`, `metti`, `arimitt`, `variabile` | `Let` | Immutable binding |
| `dicce`, `di`, `scriv`, `stampa` | `Print` | Print to stdout |
| `ie` | `If` | Conditional expression |
| `altrimenti` | `Else` | Else branch (mandatory) |
| `firmete` | `End` | Closes `ie` and `quinde` |
| `quinde` | `Function` | Function definition |
| `tornete` | `Return` | Return value from function |
| `vale`, `diventa` | `AssignWord` | Optional separator in `mitt x vale 5` |
| `chiu` | `Plus` | Addition / string concat |
| `meno` | `Minus` | Subtraction |
| `pe` | `Star` | Multiplication |
| `spart`, `divise` | `Slash` | Division |
| `uguale` | `EqualEqual` | Equality comparison |
| `diverse` | `NotEqual` | Inequality |
| `meno de` / `meno_de` | `Less` | Less than |
| `meno uguale` / `meno_uguale` | `LessEqual` | Less or equal |
| `chiu de` / `chiu_de` | `Greater` | Greater than |
| `chiu uguale` / `chiu_uguale` | `GreaterEqual` | Greater or equal |
| `e` | `And` | Logical and |
| `o` | `Or` | Logical or |
| `ne` | `Not` | Logical not |
| `sci`, `vero` | `True` | Boolean true |
| `no`, `falso` | `False` | Boolean false |
| `mbe` | `Lambda` | Anonymous function (lambda expression) |
| `po` | `Po` | Required separator between `ie` condition and then-branch |

## AST node reference

### Expressions (`Ast.hpp`, `ExprKind`)

| Node | Fields | Notes |
|---|---|---|
| `NumberExpr` | `value: string` | Numeric literal |
| `StringExpr` | `value: string` | String literal (escape-processed) |
| `BooleanExpr` | `value: bool` | `sci`/`no` |
| `IdentifierExpr` | `name: string` | Variable or function reference |
| `UnaryExpr` | `op: TokenKind`, `right` | `-` or `ne` |
| `BinaryExpr` | `op: TokenKind`, `left`, `right` | Arithmetic, comparison, logic |
| `CallExpr` | `callee: unique_ptr<Expr>`, `args` | Function call via `Scioh::apply()` |
| `IfExpr` | `condition`, `thenBranch: vector<Stmt>`, `elseBranch` | Conditional expression (IIFE lambda) |
| `LambdaExpr` | `params: vector<string>`, `body: unique_ptr<Expr>` | Anonymous function; uses `[=]` capture for closures; closes with `firmete` |

### Statements (`Ast.hpp`, `StmtKind`)

| Node | Fields | Notes |
|---|---|---|
| `LetStmt` | `name: string`, `value: Expr` | Immutable binding |
| `PrintStmt` | `value: Expr` | Prints to stdout |
| `FunctionStmt` | `name`, `params: vector<string>`, `body: vector<Stmt>` | Function definition |
| `ReturnStmt` | `value: Expr` | Generates `throw Scioh::ReturnValue{...}` |
| `ExprStmt` | `expr: Expr` | Expression used as statement (e.g., function call for side effects) |

## Codegen invariants — do not break

### `tornete a` → `throw`, not `return`

`ReturnStmt` generates `throw Scioh::ReturnValue{expr}`. Every function lambda wraps its body in:
```cpp
try { ... } catch (const Scioh::ReturnValue& ret) { return ret.value; }
```
This is required so early return propagates correctly through nested `ie` IIFE lambdas.
**Do not change `ReturnStmt` to generate a C++ `return`.**

### `ie` → immediately-invoked lambda (IIFE)

`IfExpr` generates:
```cpp
[&]() -> Scioh::Value {
    if (Scioh::isTruthy(cond)) {
        /* thenBranch statements */
        return last_expr;  // last ExprStmt becomes return
    } else {
        /* elseBranch statements */
        return last_expr;
    }
}()
```
`emitBranchBody()` handles the "last ExprStmt → return" conversion.

### Function symbols are separate from variable scopes

- Variables: `scopes_` (stack of `vector<Symbol{sourceName, cppName}>`), cppName = `v0`, `v1`, …
- Functions: `functionSymbols_` (`unordered_map<string,string>`), cppName = `"fn_" + sourceName`
- `symbolFor()` checks scopes first, then `functionSymbols_`
- **Do not merge these two registries.**

### Forward declarations before lambda assignments

All `quinde` functions are forward-declared as `Scioh::Value fn_name;` at the top of `main()` before any lambda is assigned. This enables mutual recursion via `[&]` capture by reference.

### Generated C++ variable names

- User variables: `v0`, `v1`, `v2`, … (auto-incremented `nextSymbol_`)
- Function values: `fn_<sourceName>` (literal, not auto-generated)
- **Never emit raw source names as C++ identifiers** (they may clash with C++ keywords or each other).

## How to add a new feature

### Adding a new keyword synonym

Only `Lexer.cpp` needs to change:
1. `keywordKind()`: add `if (text == "newword") return TokenKind::ExistingToken;`
2. No other files need updating.

### Adding a new operator

1. `Token.hpp`: add to `TokenKind` if needed.
2. `Lexer.cpp`: `keywordKind()` and `tokenKindName()`.
3. `Parser.cpp`: add at the correct precedence level (`orExpression` > `andExpression` > `equality` > `comparison` > `term` > `factor` > `unary` > `primary`).
4. `Codegen.cpp`: add to `opFunction()` or `unaryFunction()`.

### Adding a new statement

1. `Token.hpp`: new `TokenKind` if needed.
2. `Lexer.cpp`: `keywordKind()`, `tokenKindName()`.
3. `Ast.hpp`: new `StmtKind` entry + new struct inheriting `Stmt`.
4. `Parser.hpp`: declare new parse method.
5. `Parser.cpp`:
   - `statement()`: add check for the new token.
   - Implement the parse method.
   - `finishStatement()`: add the new token if it's a valid implicit statement terminator.
6. `Codegen.cpp`: add a `case StmtKind::NewStmt:` in `emitStatement()`.

### Adding a new expression

1–4 same as above but add to `ExprKind` and struct inheriting `Expr`.
5. `Parser.cpp`: add in `primary()` or appropriate precedence function.
6. `Codegen.cpp`: add a `case ExprKind::NewExpr:` in `emitExpr()`.

## What NOT to do

- **Do not use `cmake` or `ninja`** — not available. Use the manual `c++` command.
- **Do not add `mentre` (while loop)** — removed by design.
- **Do not add mutable assignment** — the language is purely functional.
- **Do not make `altrimenti` optional** — `ie` without `altrimenti` is not valid in a purely functional language.
- **Do not make `po` optional** — it is a required separator after the `ie` condition to disambiguate the condition from the then-branch.
- **Do not change `ReturnStmt` codegen to emit C++ `return`** — it must emit `throw ReturnValue{}`.
- **Do not modify the Scioh runtime boilerplate** in `Codegen.cpp` unless adding a new type or operator.
- **Do not add nested function definitions** (functions inside `quinde` bodies) — the codegen does not support this.
- **Do not use bare source names as C++ identifiers** in the generated output.

## Testing checklist after any change

```sh
# 1. Rebuild
cd /workspace/build
/usr/bin/c++ -std=c++20 -I../src -o sci-oh \
  ../src/main.cpp ../src/scioh/Lexer.cpp \
  ../src/scioh/Parser.cpp ../src/scioh/Codegen.cpp

# 2. Run functional example
./build/sci-oh examples/funzionale.sci -o /tmp/t && /tmp/t

# 3. Run basic example
./build/sci-oh examples/ciao.sci -o /tmp/t && /tmp/t

# 4. Run lambda example (tests mbe/fine closures and ie+po fix)
./build/sci-oh examples/lambda.sci -o /tmp/t && /tmp/t
# valore_assoluto(meno 7) must print 7, not "no"

# 5. Check immutability error
echo 'mitt x 5
x vale 10' | ./build/sci-oh /dev/stdin

# 6. Inspect generated C++ if something looks wrong
./build/sci-oh --emit-cpp examples/funzionale.sci
```
