"""
Summary:
Suppose you have the fraction like 49/98 where you can simplify it
by removing the same digit in the numerator and the denominator,
49/98 = 4/8 = 1/2 which you get by cancelling out the 9's

There are 4 non-trivial examples, ignoring fractions like 30/50 containing
2 digits in both the numerator and denominator and less than 1.

What is the denominator of the product of the fractions, in simplest form?
"""
#For digits, n,x,y, the only nontrivial cases of canceling happening is
# (10x + n)/(10n + y) = x/y.
#Solving for n, n = 9xy / (10x - y) and  0 < x,y < 10


def GCF(a,b):
    return a if b==0 else GCF(b,a%b)


num = 1
den = 1
for x in range(1,10):
    for y in range(1,10):
        #check if n is valid
        n = 9*x*y // (10*x - y)
        if n * (10*x - y) == 9*x*y and x!=y and n < 10:
            num *= 10*x + n
            den *= 10*n + y
gcf = GCF(num,den)
print(den)