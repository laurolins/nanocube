

fileall = open('crime50k.csv')
file1 = open('crime1.csv', 'w+')
file2 = open('crime2.csv', 'w+')
file3 = open('crime3.csv', 'w+')

header = fileall.readline()
file1.write(header+'\n')
file2.write(header+'\n')
file3.write(header+'\n')

i=0
for line in fileall:
	if i % 3 == 0:
		file1.write(line+'\n')
	elif i % 3 == 1:
		file2.write(line+'\n')
	else:
		file3.write(line+'\n')
	i+=1

file1.close()
file2.close()
file3.close()
fileall.close()