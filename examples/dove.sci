# dove: binding locali alla fine de quinde (stile where)

# Costanti locali
quinde cerchio(r)
    tornete a superficie piu perimetro
dove superficie vale pi pe r pe r
dove perimetro vale 2 pe pi pe r
dove pi vale 3.14159
firmete

dicce cerchio(5)

# dove che rinomina una condizione
quinde fattoriale(n)
    tornete a se caso_base po
        1
    altrimenti
        n pe fattoriale(n meno 1)
    firmete
dove caso_base vale n meno uguale 1
firmete

dicce fattoriale(6)

# dove con lambda locale
quinde raddoppia_lista(lista)
    tornete a simele lista
        cusci [] po []
        cusci [h | t] po doppia(h) mitta prime raddoppia_lista(t)
    firmete
dove doppia vale mbe(x) x pe 2 firmete
firmete

dicce raddoppia_lista([1, 2, 3, 4])

# dove con calcoli intermedi interdipendenti
quinde ipotenusa(a, b)
    tornete a radice quadrata somma_quadrati
dove somma_quadrati vale qa piu qb
dove qa vale a pe a
dove qb vale b pe b
firmete

dicce ipotenusa(3, 4)
