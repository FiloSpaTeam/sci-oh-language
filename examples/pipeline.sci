# Operatore pipe |> e funzioni built-in

mitte doppio mbe(x) x pe 2 firmete
mitte pari  mbe(x) x % 2 uguale 0 firmete
mitte somma mbe(a, b) a piu b firmete

mitte numeri [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

# mappa, filtre, pieghe
dicce mappa doppio numeri
dicce filtre pari numeri
dicce pieghe 0 somma numeri

# Catena con |>: pari raddoppiati, poi somma
dicce numeri |> filtre pari |> mappa doppio |> pieghe 0 somma

# passanne e' sinonimo di |>
dicce numeri passanne filtre pari passanne mappa doppio passanne pieghe 0 somma

# inversa, pijje, lasse
dicce inversa numeri
dicce pijje 3 numeri
dicce lasse 7 numeri

# uni
mitte parole ["San", "Benedetto", "del", "Tronto"]
dicce uni " " parole

# assol, massime, mineme, putenze
dicce assol (meno 42)
dicce massime 3 7
dicce mineme 3 7
dicce putenze 2 10
