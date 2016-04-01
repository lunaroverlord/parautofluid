import sys
import os
import subprocess

i = sys.stdin
o = sys.stdout

memcpy = {"cpu": 0.0, "gpu": 0.0}
kernels = {"cpu": 0.0, "gpu": 0.0}

for line in i:
    fields = line.strip().split(',')

    if fields[0] == "method" or fields[0][0] == '#':
        continue
    elif fields[0] == "memcpyHtoDasync":
        memcpy["gpu"] = memcpy["gpu"] + float(fields[1])
        memcpy["cpu"] = memcpy["cpu"] + float(fields[2])
    else:
        kernels["gpu"] = kernels["gpu"] + float(fields[1])
        kernels["cpu"] = kernels["cpu"] + float(fields[2])

o.write(str(memcpy) + '\n' + str(kernels))
