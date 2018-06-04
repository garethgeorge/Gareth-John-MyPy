def _in(val, list):
    for val2 in list:
        if val == val2:
            return True 
    return False

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

primes = primeSieve(1001)

maxlen = 0
max_a  = 0
max_b  = 0
for a in range(-999, 1000):
    for b in primes: #b must be prime as f(0) = prime
        count=0
        #count number of prime values of func
        for num in range(500):
            val = num*num + num*a + b
            if _in(val, primes):
                count += 1
            else:
                break
        
        #check if largest length
        if count>maxlen:
            maxlen = count
            maxa = a
            maxb = b

print("Answer",maxa*maxb)
