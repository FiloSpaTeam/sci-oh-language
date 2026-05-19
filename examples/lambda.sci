# Lambda (mbe) in sci-oh

# Lambda semplice: valore come variabile
mitte doppio mbe(x) x pe 2 firmete
dicce doppio(5)

# Lambda con due parametri
mitte somma mbe(a, b) a piu b firmete
dicce somma(3, 7)

# Lambda passata come argomento
quinde applica(f, x)
    tornete a f(x)
firmete

dicce applica(doppio, 9)

# Composizione di funzioni
quinde componi(f, g)
    tornete a mbe(x) f(g(x)) firmete
firmete

mitte triplo mbe(x) x pe 3 firmete
mitte seivote componi(doppio, triplo)
dicce seivote(4)

# Closure: la lambda cattura la variabile dall'ambiente
mitte base 100
mitte aggiungi mbe(x) x piu base firmete
dicce aggiungi(5)

# Lambda con se come corpo
mitte valore_assoluto mbe(x) se x meno de 0 po
    meno x
altrimenti
    x
firmete firmete

dicce valore_assoluto(meno 7)
dicce valore_assoluto(3)

# Funzione che ritorna una lambda (closure su parametro)
quinde adder(n)
    tornete a mbe(x) x piu n firmete
firmete

mitte piu10 adder(10)
mitte piu20 adder(20)
dicce piu10(5)
dicce piu20(5)
