#!/usr/bin/env python2
# coding: utf-8

# In[49]:


import subprocess
import sys
from PIL import Image

dir_name = sys.argv[1]

left = dir_name+'/left/'
right = dir_name+'/right/'
width = 640

def read_directory(dirname):
    p = subprocess.Popen(["ls", dirname], stdout=subprocess.PIPE)
    return p.stdout.read().split('\n')[:-1]

files_left = read_directory(left)
files_right = read_directory(right)

times_left = [d[-22:-4] for d in files_left]
times_right = [d[-22:-4] for d in files_right]

l, r = 0, 0
times_unified = list()
files_unified = list()

while True:
    if times_left[l] < times_right[r]:
        while l < len(times_left)-1 and times_left[l+1] < times_right[r]:
            l += 1

    if times_left[l] > times_right[r]:
        while r < len(times_right)-1 and times_left[l] > times_right[r+1]:
            r += 1

    times_unified.append(times_left[l])
    files_unified.append((l, r))

    l += 1
    r += 1

    if l >= len(times_left) or r >= len(times_right):
        break

with open('times.txt', 'w+') as f:
    for time in times_unified:
        f.write(str(time)+'\n')

for d in zip(files_unified, times_unified):
    lr, time = d
    li, ri = lr

    img = Image.open(left+files_left[li])
    img = img.rotate(-90, expand=True)
    img.save(left+str(time)+'.png')

    img = Image.open(right+files_right[ri])
    img = img.rotate(-90, expand=True)
    img.save(right+str(time)+'.png')

# files_left = read_directory('left')
# files_right = read_directory('right')

# for fl, fr in zip(files_left, files_right):
#     img = Image.open(left+'/'+fl)
#     img = img.rotate(-90, expand=True)
#     img.save(left+'/'+fl)

#     img = Image.open(right+'/'+fr)
#     img = img.rotate(-90, expand=True)
#     img.save(right+'/'+fr)


# In[ ]:




