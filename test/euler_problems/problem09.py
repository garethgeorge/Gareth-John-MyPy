"""
https://projecteuler.net/problem=9
A Pythagorean triplet is a set of three natural numbers, a < b < c, for which,
a^2 + b^2 = c^2
For example, 3^2 + 4^2 = 9 + 16 = 25 = 5^2.
There exists exactly one Pythagorean triplet for which a + b + c = 1000.
Find the product abc.
"""

product=0
for side1 in range(1, 501):
    for side2 in range(1, side1):
        side3 = 1000 - side1 - side2                #perimeter = 1000
        if( side1 ** 2 + side2 ** 2 == side3 ** 2): #check if right triangle
            product = side1 * side2 * side3
            break

print("The answer is",product)