#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import string

NUM_PLAYERS = 4
conv = [0,1,3,6]

if len(sys.argv) < 2:
	print "usage:",sys.argv[0],"score.log"
	exit(1)

f = open(sys.argv[1],"r")
score_sum = [0 for i in xrange(NUM_PLAYERS)]
direct_sum = [0 for i in xrange(NUM_PLAYERS)]
for line in f:
	scores = string.split(line)
	scores = map(lambda x:int(x), scores)
	sorted_s = sorted(scores)

	for i in xrange(NUM_PLAYERS):
		direct_sum[i] += scores[i]
		for j in xrange(NUM_PLAYERS):
			if scores[i] == sorted_s[j]:
				score_sum[i] += conv[j]
				break
print "score:", score_sum
print "sum:", direct_sum