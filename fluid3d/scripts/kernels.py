#!/usr/bin/python

import sys
import os
import subprocess

i = sys.stdin
o = open("kernels.dat", "w")

# arguments
draw = False
normalize = False
sequential = False
for a in sys.argv:
    if a == "-draw":
        draw = True
    elif a == "-normalize":
        normalize = True
    elif a == "-sequential":
        sequential = True

if sequential:
    states = ["frame", "fluid"]
    switches = ["fluid", "Frame"]
else:
    states = ["frame", "velocity-addSource", "velocity-diffuse", "velocity-project1", "velocity-advect", "velocity-project2", "density-addSource", "density-diffuse", "density-advect"]
    switches = ["addSource", "diffuse", "project1", "advect", "project1", "addSource", "diffuse", "advect", "Frame"]
skips = ["resample"]
state_index = 0

for s in states:
    o.write(s + ' ')
o.write('\n')

def advance():
    global state_index
    state_index += 1
    if state_index == len(states):
        state_index = 0

def state():
    return states[state_index]

total = 0
points = 0
highest = 0
linesum = 0
for line in i:
    fields = line.strip().split()

    if fields[0] in skips:
        continue

    if fields[0] == switches[state_index]: #next stage
        linesum += total
        o.write(str(total) + ' ')
        if state_index == len(states) - 1:
            o.write('\n')
            points += 1
            highest = max(highest, linesum)
            linesum = 0
        total = 0
        advance()

    total += float(fields[1]) / 1000000 #convert to ms

o.write(str(total) + "\n")
o.write("# points: " + str(points))
o.close()

# gnuplot setup
maxy = highest
drawdata = "kernels.dat"
fields = 2 if sequential else 9

if normalize:
    os.system("python normalize.py < kernels.dat > kernelsnormal.dat")
    drawdata = "kernelsnormal.dat"
    maxy = 1

print points, fields, maxy, drawdata
if draw:
    subprocess.call(["gnuplot", "-persist", "-e",
        "maxx=" + str(points) + "; maxy=" + str(maxy) + "; data='" + drawdata + "'; fields=" + str(fields),
        "kernels.gp"])
