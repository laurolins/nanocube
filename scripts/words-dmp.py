#!env python
import random
import sys

max_words = 1000000
max_rep = max([1, max_words * 0.05])
word_file = '/usr/share/dict/words'
words = open(word_file).read().splitlines()

# print output stream
count = 0
while(True):
    index = random.randint(0, len(words))
    rep = random.randint(0, max_rep)

    for i in range(0, rep):
        sys.stdout.write(words[index]+'\n')
        count+=1

        if(count >= max_words):
            exit(0)
