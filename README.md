# sci-oh

`sci-oh` e' nu linguagge funzionale puro che vo parla' lu piu' pussibile come se parla a San Benedetto del Tronto.
Lu compilatore sta scritte in C++ e traduce i file `.sci` in C++ che poi compila lu compilatore de sistema.

> Lu compilatore che compila quann ie pare

---

## Come se tira su

Te serve nu compilatore C++ che capisce C++20.

**Con CMake:**
```sh
cmake -S . -B build
cmake --build build
```

**Senza CMake:**
```sh
/usr/bin/c++ -std=c++20 -I src -o build/sci-oh \
  src/main.cpp src/scioh/Lexer.cpp src/scioh/Parser.cpp src/scioh/Codegen.cpp
```

## Come se usa

```sh
# Compila e mande
./build/sci-oh examples/ciao.sci -o ciao && ./ciao

# Vede lu C++ generate (utile pe' capisce)
./build/sci-oh --emit-cpp examples/ciao.sci

# Versione e motto
./build/sci-oh --version
```

---

## Nu programma de esempio

```sci
mitte nome "San Benedetto"
dicce "Oh, " piu nome

mitte eta 20
dicce se eta piu de 17 po
    "adulto"
altrimenti
    "minore"
firmete

quinde fattoriale(n)
    tornete a se n meno uguale 1 po
        1
    altrimenti
        n pe fattoriale (n meno 1)
    firmete
firmete

dicce fattoriale 5
```

---

## Lu linguagge

### Paradigma

sci-oh e' **puramente funzionale**:

- **Binding immutabile** — `mitte x 5` lega `x` a `5` per sempre. Riscrivere `x` e' nu errore de compilazione.
- **Niente cicli** — si usa la ricorsione. Tutti i `tornete a` in coda vengono ottimizzati (TCO).
- **`se` e' na espressione** — `altrimenti` e' sempre obbligatorie. Ritorna lu valore de lu ramo scelto.
- **Funzioni come valori** — nu nome de funzione e' nu valore passabile a un'altra funzione.
- **Chiamata per giustapposizione** — `fib 10` e `fib(10)` so' equivalenti; `f a b c` chiama `f` cu tre argomenti.

### Parole chiave

| Parola/e dialettale | Significato |
|---|---|
| `mitte`, `metti`, `remette`, `variabile` | Binding immutabile |
| `dicce`, `di`, `scrive`, `stampa` | Stampa su stdout |
| `se … po … altrimenti … firmete` | Espressione condizionale |
| `quinde nome(params) … firmete` | Definizione funzione |
| `tornete a expr` | Valore de ritorno |
| `mbe(params) expr firmete` | Lambda (funzione anonima) |
| `f x y z` oppure `f(x, y, z)` | Chiamata funzione (entrambe le forme valide) |
| `sci`, `vero` | Booleano vero |
| `no`, `falso` | Booleano falso |
| `piu`, `meno`, `pe`, `scumpunne` | Aritmetica |
| `%` | Modulo |
| `uguale`, `diverse` | Uguale / diverso |
| `meno de`, `meno uguale` | Minore / minore o uguale |
| `piu de`, `piu uguale` | Maggiore / maggiore o uguale |
| `e`, `o`, `ne` | E logico / o logico / not |
| `prime lista` | Primo elemento |
| `uldeme lista` | Coda (lista senza il primo) |
| `vute lista` | Sci se la lista e' vota |
| `elem mitta prime lista` | Prependi un elemento |
| `[ a, b, c ]` | Lista letterale |
| `simele val … cusci pattern po … firmete` | Pattern matching |
| `damme` | Legge na riga da stdin |
| `numere expr` | Converte stringa in numero |
| `quante expr` | Lunghezza de stringa o lista |
| `spezza iecch str sep` | Divide na stringa |
| `cala`, `suva`, `arretunne` | Floor, ceil, round |
| `radice quadrata expr` | Radice quadrata |
| `vabbone(expr)` | Risultato ok |
| `guaje(msg)` | Risultato errore |
| `prove expr` | Cattura eccezioni runtime in un Result |
| `dove nome vale expr` | Binding locale alla fine de quinde |

I simboli `+`, `-`, `*`, `/`, `=`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `;`, `(`, `)`, `,` funzionano pure loro.

### Pattern matching

```sci
simele lista
    cusci [] po "vuta"
    cusci [h | t] po "testa: " piu h
firmete

simele n
    cusci 0 po "zero"
    cusci * po "altro"
firmete

simele risultato
    cusci vabbone(x) po dicce x
    cusci guaje(msg) po dicce "Errore: " piu msg
firmete
```

### Gestione degli errori

```sci
quinde dividi(a, b)
    tornete a se b uguale 0 po
        guaje("divisione per zero")
    altrimenti
        vabbone(a / b)
    firmete
firmete

mitte r dividi 10 0
simele r
    cusci vabbone(x) po dicce x
    cusci guaje(msg) po dicce "Errore: " piu msg
firmete

# prove cattura eccezioni runtime (es. radice quadrata de nu numero negativo)
mitte s prove radice quadrata meno 4
simele s
    cusci vabbone(x) po dicce x
    cusci guaje(msg) po dicce "Errore: " piu msg
firmete
```

---

## Esempi

| File | Cosa mostra |
|---|---|
| `examples/ciao.sci` | Primi passi: binding, se, ricorsione |
| `examples/funzionale.sci` | Funzioni, ricorsione, valori first-class |
| `examples/lambda.sci` | Lambda, chiusure, funzioni di ordine superiore |
| `examples/liste.sci` | Liste, prime/uldeme/vute, cons |
| `examples/match.sci` | Pattern matching su liste, numeri, booleani |
| `examples/stdlib.sci` | quante, %, cala/suva/arretunne, spezza iecch |
| `examples/dove.sci` | Binding locali con dove (stile where) |
| `examples/errori.sci` | vabbone, guaje, prove |
| `examples/fibonacci.sci` | Ricorsione naïve vs TCO con accumulatore |

---

## English Version

`sci-oh` is a purely functional language that sounds as close as possible to the San Benedetto del Tronto dialect (Marche, Italy). The compiler is written in C++ and translates `.sci` files to C++, then invokes the system C++ compiler.

> The compiler that compiles whenever it feels like it

### Build

Requires a C++20-capable compiler.

**With CMake:**
```sh
cmake -S . -B build && cmake --build build
```

**Without CMake:**
```sh
/usr/bin/c++ -std=c++20 -I src -o build/sci-oh \
  src/main.cpp src/scioh/Lexer.cpp src/scioh/Parser.cpp src/scioh/Codegen.cpp
```

### Usage

```sh
./build/sci-oh examples/ciao.sci -o ciao && ./ciao
./build/sci-oh --emit-cpp examples/ciao.sci
./build/sci-oh --version
```

### Design

- **Immutable bindings** — `mitte x 5` binds `x` permanently. Rebinding is a compile-time error.
- **No loops** — use recursion. Tail calls are optimized via a trampoline.
- **`se` is an expression** — `altrimenti` is always required; returns the chosen branch value.
- **First-class functions** — a function name evaluates to a callable `Value`.
- **Result type** — `vabbone(v)` / `guaje(msg)` for ok/error; `prove` wraps runtime exceptions.
- **Juxtaposition calls** — `f x y z` and `f(x, y, z)` are equivalent. The distinction: `f(x)` (no space before `(`) uses explicit syntax; `f x` or `f (x)` (space) uses juxtaposition.

All keywords are mapped in `src/scioh/Lexer.cpp → keywordKind()`.
