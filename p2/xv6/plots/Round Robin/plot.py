#!/usr/bin/python
import argparse
import matplotlib.pyplot as plt

# Specify cmdline args
parser = argparse.ArgumentParser()
default = 'You should really put in a title and label axes!'
parser.add_argument('-i', '--input', nargs='+', help='Input File(s)')
parser.add_argument('-o', '--output', help='Output File')
parser.add_argument('-t', '--title', default=default, help='Plot title')
parser.add_argument('-x', '--xlabel', default=default, help='Label for x axis')
parser.add_argument('-y', '--ylabel', default=default, help='Label for y axis')

# Parse arguments
args = parser.parse_args()
inFiles = args.input
outFile = args.output
title = args.title
xlabel = args.xlabel
ylabel = args.ylabel

# Extract info from each input file
xs = []
ys = []
for fname in inFiles:
    x = []
    y = []
    with open(fname, 'r') as f:
        for line in f:
            data = line.strip().split(',')
            x.append(float(data[0]))
            y.append(float(data[1]))
    xs.append(x)
    ys.append(y)

# Generate plots
plots = []
min_x = min(xs[0])
min_y = min(ys[0])
max_x = max(xs[0])
max_y = max(ys[0])
for x, y in zip(xs, ys):
    min_x = min(min_x, min(x))
    min_y = min(min_y, min(y))
    max_x = max(max_x, max(x))
    max_y = max(max_y, max(y))
    plot, = plt.step(x, y)
    plots.append(plot)

# Set 5% margin
plt.xlim((min_x - .05*(max_x - min_x), max_x + .05*(max_x - min_x)))
plt.ylim((min_y - .05*(max_y - min_y), max_y + .05*(max_y - min_y)))

# Set specified title/axes labels & legend
plt.legend(plots, inFiles, loc = 'best')
plt.title(title)
plt.xlabel(xlabel)
plt.ylabel(ylabel)

# Either save or show plot
if outFile != None:
    plt.savefig(outFile)
else:
    plt.show()
