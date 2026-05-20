# Applicazione parziale (currying)

mitte doppio mbe(x) x pe 2 firmete
mitte pari   mbe(x) x % 2 uguale 0 firmete
mitte somma  mbe(a, b) a piu b firmete

mitte numeri [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

# Funzioni parzialmente applicate dalla prelude
mitte raddoppia    mappa doppio    # Lista -> Lista
mitte solo_pari    filtre pari     # Lista -> Lista
mitte aggiungi_3   somma 3        # Num -> Num

dicce raddoppia numeri
dicce solo_pari numeri
dicce aggiungi_3 7

# Funzione che ritorna una funzione (closure)
quinde moltiplica(n)
    tornete a mbe(x) x pe n firmete
firmete

mitte triplo     moltiplica 3
mitte quadruplo  moltiplica 4

dicce triplo 5
dicce mappa triplo numeri

# pieghe parziale: pieghe 0 somma e' una Lista -> Num
mitte somma_lista pieghe 0 somma
dicce somma_lista numeri

# composizione con |>
dicce numeri |> filtre pari |> mappa (moltiplica 3) |> pieghe 0 somma
