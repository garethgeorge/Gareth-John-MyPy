"""
Project Euler #10
https://projecteuler.net/problem=10

The sum of the primes below 10 is 2 + 3 + 5 + 7 = 17.

Find the sum of all the primes below two million.
"""


#returns list of primes <= limit
def enumerate(gen):
  x = 0
  for val in gen:
    yield x, val
    x += 1

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

#returns list of primes <= limit
def primeSieve(limit):
    if limit < 2: return []
    if limit < 3: return [2]
    size = (limit - 3) // 2
    sieve = [True] * (size + 1)

    maxint=int(limit**0.5)
    for i in range( (maxint - 1) // 2 ):
        if sieve[i]:
            prime = 2 * i + 3
            start = prime * (i + 1) + i
            sieve[start:len(sieve):prime] = [False] * ((size - start) // prime + 1)
    
    tmplist = []
    for i, v in enumerate(sieve):
        if v:
            tmplist.append(2 * i + 3)

    return [2] + tmplist

primes = primeSieve(2 * 10 ** 6)

print("Answer",sum(primes))