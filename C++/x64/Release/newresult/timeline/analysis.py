import re


def check(infile, outfile):
	#p = re.compile("(\d*): (\d*)\((d*)\) (\d*): (\d*)\((\d*)\) 	(\d*) (\d*) 0 0 112504 0 0")
	p = re.compile("(\d*): (.*): (\d*)\((\d*)\) 	(\d*) (.*)")
	p1 = re.compile("accuracy = (\d*)")
	wfile = open(outfile,"w")
	throughput = []
	accuracy = []
	numvm = []
	time = []
	vm = []
	th = []
	t = []
	
	for line in open(infile):
		m = p.match(line)
		m1 = p1.match(line)
		if m != None:
			#print m
			t = int(m.group(1))
			print t
			#vm.append(int(line.count(')')))
			vm = int(line.count(')'))
			th.append(int(m.group(5)))

			#print m.group(1), m.group(4)
			#exit(1)
		elif m1 != None:
			#print line
			
			acc = int(m1.group(1))

			accuracy.append(acc)
			#print t
			#print len(t)
			
			time.append(t)
			#print time
			#exit(1)
			#print vm
			#exit(1)
			#if len(vm) == 0:
			#	numvm.append(0)
			#else:
			#	numvm.append(vm[len(vm) - 1])
			numvm.append(vm)

			if len(th) == 0:
				throughput.append(0)
			else:
				throughput.append(sum(th)/len(th))
			#t = []
			th = []
			#vm = []
			#print numvm
			#exit(1)
	for ii in range(len(time)):
		wfile.write(str(time[ii]) + '\t' + str(numvm[ii])
				+ '\t' + str(throughput[ii]) + '\t' + str(accuracy[ii]) + '\r\n')



check("log.txt", "timeline.plo")
