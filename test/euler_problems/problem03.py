#given the number 600851475143
number=600851475143

#divide by 2 until odd or 2 (if n was originally 2 ** k )
while( number % 2 == 0 and number > 2 ):
    number%=2

#divide number by its prime factors one by one
#until only its largest prime factor remains
#rather than spending time seperating the prime numbers from the odd ones
#we will just try to divide by all odd numbers
odd=3
while odd <= number ** 0.5:
    if number % odd == 0:
        number //= odd
    else:
        odd += 2

print("Answer",number)