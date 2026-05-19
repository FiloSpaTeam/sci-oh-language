# Fibonacci in sci-oh
#
# Versione naïve O(2^n): semplice ma lenta
quinde fib(n)
    tornete a se n meno uguale 1 po
        n
    altrimenti
        fib (n meno 1) piu fib (n meno 2)
    firmete
firmete

# Versione con accumulatore O(n): tail-recursive, ottimizzata (TCO)
quinde fibVeloce(n)
    tornete a fibAcc n 0 1
firmete

quinde fibAcc(n, a, b)
    tornete a se n uguale 0 po
        a
    altrimenti
        fibAcc (n meno 1) b (a piu b)
    firmete
firmete

dicce "Versione naïve:"
dicce fib(0)
dicce fib(1)
dicce fib(10)

dicce "Versione veloce (TCO):"
dicce arretunne fibVeloce(10)
dicce arretunne fibVeloce(30)
dicce arretunne fibVeloce(50)
