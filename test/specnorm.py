#from math import sqrt

def zip(u,v):
    for ind in range(0,len(u)):
        yield (u[ind],v[ind])

def enumerate(iter):
  x = 0
  for val in iter:
    yield x, val 
    x += 1

def eval_A(i, j):
    ij = i+j
    return 1.0 / (ij * (ij + 1) / 2 + i + 1)


def eval_A_times_u(u):
    local_eval_A = eval_A

    return [ sum([ local_eval_A(i, j) * u_j
                   for j, u_j in enumerate(u)
                 ]
                )
             for i in range(len(u))
           ]


def eval_At_times_u(u):
    local_eval_A = eval_A

    return [ sum([ local_eval_A(j, i) * u_j
                   for j, u_j in enumerate(u)
                 ]
                )
             for i in range(len(u))
           ]


def eval_AtA_times_u(u):
    return eval_At_times_u(eval_A_times_u(u))


def main():
    n = 100
    u = [1] * n
    local_eval_AtA_times_u = eval_AtA_times_u

    for dummy in range(10):
        v = local_eval_AtA_times_u(u)
        u = local_eval_AtA_times_u(v)

    vBv = vv = 0

    for ue, ve in zip(u, v):
        vBv += ue * ve
        vv  += ve * ve

    print("%0.9f" % (sqrt(vBv/vv)))

main()