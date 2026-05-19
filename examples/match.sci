# Funzione ricorsiva con pattern matching su lista
quinde somma(lista)
    tornete a simele lista
        cusci [] po 0
        cusci [h | t] po h piu somma(t)
    firmete
firmete

dicce somma([1, 2, 3, 4, 5])

# Pattern su numeri
quinde descrivi(n)
    tornete a simele n
        cusci 0 po "zero"
        cusci 1 po "uno"
        cusci * po "altro"
    firmete
firmete

dicce descrivi(0)
dicce descrivi(1)
dicce descrivi(42)

# Pattern su booleani
quinde descriviBool(b)
    tornete a simele b
        cusci sci po "vero!"
        cusci no po "falso!"
    firmete
firmete

dicce descriviBool(sci)
dicce descriviBool(no)

# Lunghezza lista con match
quinde lunghezza(lista)
    tornete a simele lista
        cusci [] po 0
        cusci [h | t] po 1 piu lunghezza(t)
    firmete
firmete

dicce lunghezza([10, 20, 30])

# Inverti lista
quinde inverti(lista)
    tornete a simele lista
        cusci [] po []
        cusci [h | t] po inverti(t) piu [h]
    firmete
firmete

dicce inverti([1, 2, 3])
