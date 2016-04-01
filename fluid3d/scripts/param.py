
import sys
import os
import subprocess

i = sys.stdin
o = open("param.dat", "w")

draw = True

o.write("Nr measured time")

gap = 20
param = 5
paramsum = 0.0
maxy = 0
maxx = param #base
for line in i:
    fields = line.strip().split()

    if fields[0] == "FPS":
        continue
    elif int(fields[0]) % gap == 0 and int(fields[0]) != 0:
        if paramsum / gap > maxy:
            maxy = paramsum / gap
        o.write(str(param) + ' ' + str(paramsum / gap) + '\n')
        param = param + 1
        paramsum = 0
    else:
        paramsum = paramsum + float(fields[1])

o.close()

print maxy

#draw resolution change
drawdata = "param.dat"
if draw:
    maxx = param - maxx
    subprocess.call(["gnuplot", "-persist", "-e",
        "maxx=" + str(maxx) + "; maxy=" + str(maxy) + "; data='" + drawdata + "'",
        "param_resolution.gp"])
