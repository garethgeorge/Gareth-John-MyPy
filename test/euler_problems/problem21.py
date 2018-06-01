"""
https://projecteuler.net/problem=21
Let d(n) be defined as the sum of proper divisors of n (numbers less
than n which divide evenly into n). If d(a) = b and d(b) = a, where
a ≠ b, then a and b are an amicable pair and each of a and b are
called amicable numbers.
For example, the proper divisors of 220 are 1, 2, 4, 5, 10, 11, 20, 22,
44, 55 and 110; therefore d(220) = 284.
The proper divisors of 284 are 1, 2, 4, 71 and 142; so d(284) = 220.
Evaluate the sum of all the amicable numbers under 10000.
"""

#divs[n] = list of all factors of n
divs=[[],[]]

def sum(gen):
  acum = 0
  for val in gen:
    acum += val 
  return acum

for num in range(2,10001):
    divs.append([])
    maxFactor = int(num ** 0.5)
    for factor in range(1, maxFactor + 1):
        if num % factor == 0 and num > factor:
            divs[num] += [ factor, num // factor ]

def divsum(n):
    return sum(divs[n])-n

ans=0
for i in range(2,10001):
    #ignore perfect numbers and numbers with divisor sums > 10000
    if divsum(i)!=i and (divsum(i) <= 10000):
        #if d(d(x))=x
        if divsum( divsum(i) ) == i:
            ans+=i

print("Answer", ans)
