"""
Project Euler #31
https://projecteuler.net/problem=31

How many different ways can £2 be made using any number of coins?
(of denomination 1p, 2p, 5p, 10p, 20p, 50p, 100p, 200p)
"""


limit = 200
coins = [1,2,5,10,20,50,100,200]

#Let W(n,c) be the number of ways to make n cents with coins of value
#up to c. You can divide each combination of coins represented into
#two groups, those w/o and those with coins of value c. The number of
#combinations w/o is W(n,c-1). The other group has W(n-c,c) combinations.

#Thus W(n,c) = W(n, c-1) + W(n-c,c)

ways=[1]+[0]*limit #0 has one way to make it
for coin in coins:
    for val in range(coin,limit+1):
        ways[val]+=ways[val-coin]

print("Anser ",ways[200]," computed in ms")
