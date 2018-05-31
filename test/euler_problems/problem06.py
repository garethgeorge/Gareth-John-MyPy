"""
https://projecteuler.net/problem=6

The sum of the squares of the first ten natural numbers is,
1*1 + 2*2 + ... + 10*10 = 385
The square of the sum of the first ten natural numbers is,
(1 + 2 + ... + 10) ^ 2 = 3025
Hence the difference between the sum of the squares of the first ten
natural numbers and the square of the sum is 3025 − 385 = 2640.

Find the difference between the sum of the squares of the first
one hundred natural numbers and the square of the sum.
"""
def sumN(n):
    #based on math formula
    return (n * n + n) // 2
def sumNSquared(n):
    #based of math formula
    return (2 * n**3 + 3 * n**2 + n )//6

ans=sumN(100)**2 - sumNSquared(100)

print("Answer",ans)
    