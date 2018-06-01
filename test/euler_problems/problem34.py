"""
Find the sum of all numbers which are equal to the sum of the
factorial of their digits. (Not including 1 and 2)
"""

def factorial(i):
    return 1 if i==0 else i*factorial(i-1)

#let sum[i] = sum of factorial of digits of i and solve for single digit #s
sum = [factorial(i) for i in range(10) ]

#For number with digits ABCDEF, sum(ABCDEF) = sum(ABCDE) + sum(F)
#Dynamic programming allows for fastest computation of multi digit numbers
ans = 0
for num in range(10,10**6):
    body = num//10
    tail = num%10
    sum.append(sum[body] + sum[tail])
    if sum[num] == num:
        ans+=num
print("Answer ",ans," computed in ms")
        