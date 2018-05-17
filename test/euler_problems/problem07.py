"""
https://projecteuler.net/problem=7

By listing the first six prime numbers:
2, 3, 5, 7, 11, and 13, we can see that the 6th prime is 13.
What is the 10,001st prime number?
"""
def range(n):
  x = 0
  while x < n:
    yield n 
    x += 1

def enumerate(gen):
  x = 0
  for val in gen:
    yield x, val


def primeSieve(limit):
    if limit < 2: return []
    if limit < 3: return [2]
    #Create a sieve
    size = (limit - 3) // 2
    thing  = (size + 1)
    print("thing: " , thing)
    sieve = [True] * thing
    print("\t\tmade it!!!")
    #sieve[i] = whether (2 * i + 3) is prime
    #hence sieve[0] is for 3,
    #      sieve[1] is for 5,
    #      sieve[2] is for 7, etc
    

    #iterate over every prime <= sqrt limit
    maxint=int(limit**0.5)
    for i in range( (maxint - 1) // 2 ):
        if sieve[i]:
            prime = 2 * i + 3
            start = prime * (i + 1) + i
            sieve[start::prime] = [False] * ((size - start) // prime + 1)
    return [2] + [2 * i + 3 for i, v in enumerate(sieve) if v]


#the number of primes below N is assymptotic to N/ln(N)
#125,000 in this formula results in 10650
#this is just above 10,000 so 125,000 is a good limit for the prime sieve
#primes=primeSieve(125000)
primes=primeSieve(125000)
print("Answer is",primes[10000])