import sys 
from collections import defaultdict

with open(sys.argv[1], "r") as f:
    counts = defaultdict(int)
    durations = defaultdict(int)

    for line in f:
        try:
            line = line.split(",")
            counts[line[1]] += 1
            durations[line[1]] += float(line[-1])
        except:
            pass 

    for key in sorted(durations.keys()):
        print("%s,%f" % (key, durations[key] / float(counts[key])))
