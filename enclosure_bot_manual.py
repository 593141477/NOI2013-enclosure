#!/usr/bin/python

import sys
import random

random.seed()
x = random.randint(0,10)
y = random.randint(0,10)
(x, y) = (3, 3)
sys.stdin.readline()
sys.stdout.write("[POS] %d %d\n"%(x,y))
sys.stdout.flush()


op = 0
direction = ' '
keymap = {'w':'u', 'a':'l', 'd':'r', 's':'d', 'z':'s', ' ':1, '\r':-1}

def key_event(e):
	global op, direction

	if e.char==' ' or e.char=='\r':
		op = keymap[e.char]
		cur_action.set("[ACTION] %s %d"%(direction, op))
	else:
		char = string.lower(e.char)
		if char not in keymap:
			return
		direction = keymap[char]

		sys.stdin.readline()
		sys.stdout.write("[ACTION] %s %d\n"%(direction, op))
		sys.stdout.flush()

		cur_action.set("[ACTION] %s %d"%(direction, op))
		
		direction = ' '
		op = 0


tips = "W,A,S,D move\nZ stop\nSpace draw\nEnter trap"

import Tkinter
import string
tk_root = Tkinter.Tk()
cur_action = Tkinter.StringVar()

l = Tkinter.Label(tk_root, textvariable = cur_action)
l.grid()

l =Tkinter.Label(tk_root, text=tips)
l.bind('<Key>', key_event)
l.focus_set()
l.grid()
# tk_root.attributes('-topmost', 1)
# tk_root.focus_force()
# tk_root.lift()
tk_root.mainloop()