# sci-oh

Compiler for a purely functional language based on the Sambenedettese dialect
(San Benedetto del Tronto, Marche, Italy). `.sci` source files are translated to C++
and then compiled by the system C++ compiler.

## Building the compiler

CMake and ninja are not available in the container. Use the direct compiler command:

```sh
cd /workspace/build
/usr/bin/c++ -std=c++20 -I../src -o sci-oh \
  ../src/main.cpp \
  ../src/scioh/Lexer.cpp \
  ../src/scioh/Parser.cpp \
  ../src/scioh/Infer.cpp \
  ../src/scioh/Codegen.cpp
```

## Usage

```sh
# Compile and run
./build/sci-oh examples/funzionale.sci -o /tmp/out && /tmp/out

# Emit generated C++ (useful for debugging)
./build/sci-oh --emit-cpp examples/funzionale.sci

# Print version
./build/sci-oh --version
```

## Architecture

```
.sci source → Lexer → tokens → Parser → AST → Codegen → C++ → system c++ → binary
```

All compiler sources live in `src/scioh/`:

| File | Role |
|---|---|
| `Token.hpp` | `TokenKind` enum + `Token` struct (includes `followsWhitespace` field) |
| `Lexer.hpp/.cpp` | Tokenizer. `keywordKind()` maps all dialect words to token kinds |
| `Ast.hpp` | All AST node types (Expr and Stmt hierarchies) |
| `Parser.hpp/.cpp` | Recursive-descent parser |
| `Codegen.hpp/.cpp` | C++ code generator. Emits a single `.cpp` with the full Scioh runtime + `main()` |
| `Diagnostic.hpp` | `DiagnosticError` exception with source location |
| `SourceLocation.hpp` | `{line, column}` struct |

`src/main.cpp` handles CLI arguments, file I/O, and invocation of the system C++ compiler.

## Paradigm: purely functional

sci-oh is **purely functional**. These are firm architectural decisions, not to be reversed:

- **Immutable bindings**: `mitte x 5` is the only way to bind a name; `x vale 10` is a compile-time error.
- **No loops**: `mentre` is removed from the language. Use recursion.
- **`se` is an expression**: `altrimenti` is always required. Returns the value of the selected branch.
- **First-class functions**: a function name used without arguments evaluates to a `Scioh::Value` holding a callable.
- **Juxtaposition calls**: `f x y z` and `f(x, y, z)` are both valid. `f(x)` (no space before `(`) uses explicit call syntax; `f x` or `f (x)` (preceded by whitespace) uses juxtaposition. Arguments are collected only from the same source line as the callee.

## Call syntax

Two equivalent forms:

| Form | Example | Rule |
|---|---|---|
| Explicit | `f(x, y, z)` | `(` immediately follows the callee with **no whitespace** |
| Juxtaposition | `f x y z` | space-separated tokens on the **same source line** as the callee |

The `Token::followsWhitespace` field (set by the lexer) drives this distinction. `application()` in the parser implements both:

- If next token is `(` and `!followsWhitespace` → explicit call, collect comma-separated args until `)`.
- Otherwise, collect `primary()` expressions while they start on the same line as the callee.

Parentheses still work as grouping in juxtaposition: `fibAcc (n meno 1) b (a piu b)` passes three arguments — the parens wrap sub-expressions, not argument lists.

## Keyword reference

All keywords are mapped in `Lexer.cpp` → `keywordKind()`.

| Dialect word(s) | Token | Meaning |
|---|---|---|
| `mitte`, `metti`, `remette`, `variabile` | `Let` | Immutable binding |
| `dicce`, `di`, `scrive`, `stampa` | `Print` | Print to stdout |
| `se` | `If` | Conditional expression |
| `altrimenti` | `Else` | Else branch (mandatory) |
| `firmete` | `End` | Closes `se` and `quinde` |
| `quinde` | `Function` | Function definition |
| `tornete` | `Return` | Return a value (`tornete a expr`) |
| `vale`, `diventa` | `AssignWord` | Optional separator in `mitte x vale 5` |
| `piu` | `Plus` | Addition or string concatenation |
| `meno` | `Minus` | Subtraction |
| `pe` | `Star` | Multiplication |
| `scumpunne` | `Slash` | Division |
| `uguale` | `EqualEqual` | Equality |
| `diverse` | `NotEqual` | Inequality |
| `meno de` / `meno_de` | `Less` | Less than |
| `meno uguale` / `meno_uguale` | `LessEqual` | Less or equal |
| `piu de` / `piu_de` | `Greater` | Greater than |
| `piu uguale` / `piu_uguale` | `GreaterEqual` | Greater or equal |
| `e` | `And` | Logical and |
| `o` | `Or` | Logical or |
| `ne` | `Not` | Logical not |
| `sci`, `vero` | `True` | Boolean true |
| `no`, `falso` | `False` | Boolean false |
| `mbe` | `Lambda` | Anonymous function (lambda) |
| `po` | `Po` | Separator between `se` condition and then-branch |
| `dove` | `Dove` | Local binding at end of `quinde` (where-clause style) |
| `passanne` | `PipeRight` | Pipe operator: `a passanne f b` = `f(b, a)` (same as `\|>`) |

Symbols `+`, `-`, `*`, `/`, `=`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `;`, `(`, `)`, `,`, `|>` also work.

## Built-in prelude functions

These are available without declaration in every program:

| Function | Signature | Description |
|---|---|---|
| `mappa` | `mappa f lista` | Map `f` over every element of `lista` |
| `filtre` | `filtre f lista` | Keep elements of `lista` for which `f` returns `sci` |
| `pieghe` | `pieghe acc f lista` | Fold-left: `f(f(f(acc, x1), x2), x3)` |
| `inversa` | `inversa lista` | Reverse a list in O(n) |
| `pijje` | `pijje n lista` | First `n` elements of `lista` |
| `lasse` | `lasse n lista` | Drop first `n` elements of `lista` |
| `uni` | `uni sep lista` | Join list elements into a string with separator |
| `assol` | `assol n` | Absolute value |
| `massime` | `massime a b` | Maximum of two numbers |
| `mineme` | `mineme a b` | Minimum of two numbers |
| `putenze` | `putenze base exp` | Raise `base` to the power `exp` |

## Pipe operator

`a |> f` is equivalent to `f(a)`. When the right side has extra arguments,
the left side is appended as the **last** argument:

```sci
lista |> mappa doppio        # = mappa(doppio, lista)
lista |> filtre pari         # = filtre(pari, lista)
lista |> pieghe 0 somma      # = pieghe(0, somma, lista)
lista |> filtre pari |> mappa doppio |> pieghe 0 somma  # chained
```

`passanne` is an equivalent dialect keyword for `|>`.

## Syntax examples

```sci
# Immutable binding
mitte nome "San Benedetto"
mitte x 42

# se as expression (altrimenti mandatory, po separates condition from body)
mitte categoria se eta piu de 17 po
    "adulto"
altrimenti
    "minore"
firmete

# Function definition — both call styles valid
quinde fattoriale(n)
    tornete a se n meno uguale 1 po
        1
    altrimenti
        n pe fattoriale (n meno 1)
    firmete
firmete

# Juxtaposition calls: f x y z == f(x, y, z)
dicce fattoriale 5           # same as fattoriale(5)
dicce fattoriale(5)          # explicit form, also valid

# Multi-arg juxtaposition (same line only)
quinde fibAcc(n, a, b)
    tornete a se n uguale 0 po
        a
    altrimenti
        fibAcc (n meno 1) b (a piu b)
    firmete
firmete

# Lambda (mbe): anonymous function, single-expression body, closed by firmete
mitte doppio mbe(x) x pe 2 firmete
mitte somma mbe(a, b) a piu b firmete
dicce doppio 5      # → 10
dicce somma 3 7     # → 10

# Function as first-class value
mitte fn fattoriale
mitte risultato fn 5

# se inside dicce
dicce se x piu de 0 po
    "positivo"
altrimenti
    "non positivo"
firmete
```

## Codegen details

`Codegen.cpp` emits a **single C++ file** containing:

1. The complete Scioh runtime (in `namespace Scioh`): `Value` type, arithmetic/logic helpers, `apply()`, `ReturnValue` struct.
2. `int main()` with:
   - Forward declarations: `Scioh::Value fn_a, fn_b;` for every `quinde` definition.
   - Lambda assignments: each `quinde` becomes a `std::function` wrapped in a `Scioh::Value`.
   - Top-level statements (bindings, prints, expression statements).

### Value type

`Value` is a `std::variant`-based sum type (~40 bytes):

```
std::variant<double, bool, std::string,
             shared_ptr<FnBox>,               // function
             shared_ptr<vector<Value>>,        // list
             pair<bool, shared_ptr<Value>>>    // Result {ok, inner}
```

Recursive members (`vector<Value>`, `Value` inside Result) are heap-boxed via `shared_ptr` to avoid circular size dependency. Accessors: `kind()`, `asNumber()`, `asBoolean()`, `asString()`, `asFunction()`, `asList()`, `resultOk()`, `resultInner()`. Factory methods: `Value::ok(v)`, `Value::err(v)`.

### Key invariants

- **Symbol table** (`scopes_`): stack of scopes, each a list of `{sourceName, cppName}`. User variables get auto-generated names `v0`, `v1`, …
- **Function registry** (`functionSymbols_`): separate from scopes, maps source names to `"fn_" + name`. `symbolFor()` checks scopes first, then this registry.
- **`tornete a`** generates `throw Scioh::ReturnValue{expr}`. Each function lambda catches it with `catch (const Scioh::ReturnValue& ret)`. This ensures early return propagates correctly through nested `se` IIFE lambdas.
- **`se` codegen**: emits an immediately-invoked lambda `[&]() -> Scioh::Value { if ... else ... }()`. The last `ExprStmt` in each branch becomes `return expr;` via `emitBranchBody()`.
- **Recursion / mutual recursion**: all functions are forward-declared before any lambda is assigned, so `[&]` capture by reference works for self- and cross-calls.
- **`emitMatchBranches()`**: shared helper used by both IIFE and tail contexts to emit the if/else-if chain for `simele` pattern matching, eliminating duplication between `emitExpr` and `emitTailReturn`.

## dove (where-clause)

`dove name expr` at the end of a `quinde` body declares a local binding visible to the entire function. Multiple `dove` lines are evaluated in **reverse source order** (last declared = first evaluated), matching Haskell `where` style: write the compound expression first, building blocks below.

```sci
quinde ipotenusa(a, b)
    tornete a radice quadrata somma_quadrati
dove somma_quadrati vale qa piu qb
dove qa vale a pe a
dove qb vale b pe b
firmete
```

The function call uses juxtaposition: `radice quadrata somma_quadrati` = `radice(quadrata, somma_quadrati)` — `radice quadrata` is a two-keyword built-in parsed as a single expression, not a call.

Implemented as syntactic sugar: `dove` bindings are hoisted to `LetStmt` nodes prepended to the function body before codegen.

## Adding a new keyword

1. `Token.hpp`: add to `TokenKind` if a dedicated token is needed.
2. `Lexer.cpp` → `keywordKind()`: map the dialect word to the token kind.
3. `Lexer.cpp` → `tokenKindName()`: add the display name.
4. `Parser.hpp`: declare any new parse methods.
5. `Parser.cpp`: implement parsing. New statements go in `statement()`; new expressions go in `primary()` or the appropriate precedence level. The expression precedence chain is: `expression → or → and → not → comparison → additive → multiplicative → unary → application → primary`. Call syntax lives in `application()`.
6. `Ast.hpp`: add new node structs if needed.
7. `Codegen.cpp`: add code generation for the new node.

## Tests

With cmake/ninja available:
```sh
ctest --test-dir build --output-on-failure
```

Manual smoke tests:
```sh
./build/sci-oh examples/funzionale.sci -o /tmp/t && /tmp/t
./build/sci-oh examples/ciao.sci -o /tmp/t && /tmp/t
```

Verify error messages are clear:
```sh
echo 'mitte x 5
x vale 10' | ./build/sci-oh /dev/stdin
# expected: error containing "immutabili"
```
