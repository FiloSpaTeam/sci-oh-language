# Gestione degli errori con vabbone/guaje/prove

# Divisione sicura
quinde dividi(a, b)
    tornete a se b uguale 0 po
        guaje("divisione per zero")
    altrimenti
        vabbone(a / b)
    firmete
firmete

# Radice quadrata sicura
quinde radice_sicura(n)
    tornete a se n meno de 0 po
        guaje("numero negativo")
    altrimenti
        vabbone(radice quadrata n)
    firmete
firmete

# Funzione che elabora un risultato
quinde elabora(r)
    simele r
        cusci vabbone(x) po dicce "Risultato: " piu x
        cusci guaje(msg) po dicce "Errore: " piu msg
    firmete
firmete

elabora(dividi(10, 2))
elabora(dividi(7, 0))
elabora(radice_sicura(16))
elabora(radice_sicura(meno 4))

# prove cattura le eccezioni runtime
mitte r prove radice quadrata meno 9
simele r
    cusci vabbone(x) po dicce "Radice: " piu x
    cusci guaje(msg) po dicce "Errore catturato: " piu msg
firmete
