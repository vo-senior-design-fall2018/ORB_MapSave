#!/bin/python2

import sys

fl = open('l', 'r')
fr = open('r', 'r')
fo = open('lr', 'w+')
flf = open('lfiltered', 'w+')
frf = open('rfiltered', 'w+')

fl_lines = fl.readlines()
fr_lines = fr.readlines()

nSkip = len(fl_lines) // int(sys.argv[1])

i = 0;
for ll, lr in zip(fl_lines, fr_lines):
    if (i == 0):
        fo.write('left/'+ll)
        fo.write('right/'+lr)
        flf.write('left/'+ll)
        frf.write('right/'+lr)

    i = (i+1)%nSkip

fl.close()
fr.close()
fo.close()
flf.close()
frf.close()
