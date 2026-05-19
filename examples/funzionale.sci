# Test del compilatore sci-oh in stile funzionale

# Funzione semplice: saluto
quinde saluta(nome)
    dicce "Ciao, " piu nome piu "!"
    tornete a "salutate"
firmete

# Funzione con se come espressione che ritorna valore
quinde massimo(a, b)
    tornete a se a piu de b po
        a
    altrimenti
        b
    firmete
firmete

# Funzione ricorsiva: fattoriale
quinde fattoriale(n)
    tornete a se n meno uguale 1 po
        1
    altrimenti
        n pe fattoriale(n meno 1)
    firmete
firmete

# se come espressione in una dichiarazione
mitte eta 20
mitte categoria se eta piu de 17 po
    "adulto"
altrimenti
    "minore"
firmete

dicce "Categoria: " piu categoria

# Chiamata funzione normale
saluta("Sanbenedette")

# Funzione come valore first-class: assegna massimo a fn_max
mitte fn_max massimo

# Chiama fn_max come funzione first-class
mitte risultato_max fn_max(3, 7)
dicce "Massimo de 3 e 7: " piu risultato_max

# Calcolo fattoriale
mitte fact5 fattoriale(5)
dicce "Fattoriale de 5: " piu fact5

# se come espressione in dicce
dicce se eta piu de 18 po
    "Majenne!"
altrimenti
    "Minurenne"
firmete
