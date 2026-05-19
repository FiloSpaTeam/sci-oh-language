# quante
mitte s "San Benedetto"
dicce quante s

mitte l [10, 20, 30, 40]
dicce quante l

# modulo
dicce 17 % 5
dicce 10 % 3

# cala suva arretunne radice quadrata
dicce cala 3.7
dicce suva 3.2
dicce arretunne 3.5
dicce radice quadrata 16

# FizzBuzz
quinde fizzbuzz(n)
    tornete a se n % 15 uguale 0 po
        "FizzBuzz"
    altrimenti
        se n % 3 uguale 0 po
            "Fizz"
        altrimenti
            se n % 5 uguale 0 po
                "Buzz"
            altrimenti
                "" piu n
            firmete
        firmete
    firmete
firmete

dicce fizzbuzz(3)
dicce fizzbuzz(5)
dicce fizzbuzz(15)
dicce fizzbuzz(7)

# spezza iecch
mitte parole "uno due tre" spezza iecch " "
dicce parole
dicce prime parole
dicce quante parole
