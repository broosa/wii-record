import sys
import os

import collections

from matplotlib import pyplot as plt

t_buffer = collections.deque(maxlen=1000)
val_buffer = collections.deque(maxlen=1000)

fig, axis = plt.subplots()
li, = axis.plot(t_buffer, val_buffer);


while True:
    line_input = input()

    e_type, tstamp, x, y, z = line_input.split(",");

    if e_type == 1:
        t_buffer.append(tstamp)
        val_buffer.append(x);

        li.set_xdata(t_buffer)
        li.set_ydata(val_buffer)
        fig.draw()
