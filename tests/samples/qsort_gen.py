import os
import sys
from random import random as rand
from math import floor

def gen_rand_num(beg, end):
    val = beg + rand() * (end - beg)
    val = int(floor(val))
    return val

def gen_rand_arr(beg, end, size):
    return map(lambda x: gen_rand_num(beg, end), [0] * size)

MAX = 100000

if __name__ == "__main__":
    n = int(sys.argv[1])
    arr = gen_rand_arr(-MAX, MAX, n)
    f = open("qsort.in", "w")
    for x in arr:
        f.write(str(x) + " ")
    f.write("\n")
    f.close()
