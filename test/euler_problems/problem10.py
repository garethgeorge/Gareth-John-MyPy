"""
Project Euler #10
https://projecteuler.net/problem=10

The sum of the primes below 10 is 2 + 3 + 5 + 7 = 17.

Find the sum of all the primes below two million.
"""

def sum(gen):
  acum = 0
  for val in gen:
    acum += val 
  return acum


"""
Project Euler #10
https://projecteuler.net/problem=10

The sum of the primes below 10 is 2 + 3 + 5 + 7 = 17.

Find the sum of all the primes below two million.
"""

def enumerate(iter):
  x = 0
  for val in iter:
    yield x, val 
    x += 1

def primeSieve(limit):
    if limit < 2: return []
    if limit < 3: return [2]
    #Create a sieve
    size = (limit - 3) // 2
    thing  = (size + 1)
    sieve = [True] * thing

    maxint=int(limit**0.5)
    for i in range( (maxint - 1) // 2 ):
        if sieve[i]:
            prime = 2 * i + 3
            start = prime * (i + 1) + i

            for x in range(start, -1, prime):
                sieve[x] = False

    tmplist = []
    for i, v in enumerate(sieve):
        if v:
            tmplist.append(2 * i + 3)

    return [2] + tmplist

primes = primeSieve(2 * 10 ** 6)

print("Answer",sum(primes))