#normalizes
import sys

i = sys.stdin
o = sys.stdout

def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        pass
 
    try:
        import unicodedata
        unicodedata.numeric(s)
        return True
    except (TypeError, ValueError):
        pass
 
    return False

for line in i:
    fields = line.strip().split()
    if is_number(fields[0]):
        fields = map(float, fields)
        o.write(str(fields[0]) + ' ')
        ceiling = sum(fields[1:])
        #o.write('ceil: ' + str(ceiling) + '\n')
        for f in fields[1:]:
            o.write(str(float(f) / ceiling) + ' ')
    else:
        o.write(' '.join(fields))
    o.write('\n')

