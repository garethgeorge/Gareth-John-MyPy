"""
https://projecteuler.net/problem=7

By listing the first six prime numbers:
2, 3, 5, 7, 11, and 13, we can see that the 6th prime is 13.
What is the 10,001st prime number?
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

#the number of primes below N is assymptotic to N/ln(N)
#125,000 in this formula results in 10650
#this is just above 10,000 so 125,000 is a good limit for the prime sieve
primes=primeSieve(125000)
# print(primes)
# primes=primeSieve(125000)
print("Answer is",primes[10000])