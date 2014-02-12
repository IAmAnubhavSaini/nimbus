import re


total_log = open("count_total.plo","w")

data = [[] for ii in range(6)]   #60 sec 1%
data1 = [[] for ii in range(6)] # 10 sec 1%
data2 = [[] for ii in range(6)] # 10 sec 1% standby
data3 = [[] for ii in range(6)] # 120 sec 1%

data4 = [[] for ii in range(6)] # 10 sec 1% standby
data5 = [[] for ii in range(6)] # 60 sec 1% standby


def getkey(val):
	array = [0, 1000, 2000, 4000, 6000, 9000]
	for ii in range(len(array)):
		if array[ii] == val:
			return ii
	return -1

def getval(key):
	array = [0, 1000, 2000, 4000, 6000, 9000]
	return array[key]
	#return -1

def logresult(acc, total_vm):
	global total_log
	#print acc
	total_log.write(str(acc) + '\t' + str(total_vm) + '\r\n')

def getresult_4(infile, key, flag):
	global data

	total_vm = 0
	start = 0
	acc = []
	p = re.compile("accuracy = (\S*)")
	for line in open(infile):
		if "cmd" in line:
			total_vm += 1
		elif "start tg 4" in line:
			start = 1
		elif "stop tg 4" in line:
			start = 0
		else:
			m = p.match(line)
			if (m != None and start == 1):
				val = int(m.group(1))
				acc.append(val)
	#total = sum(acc)
	acc = sum(acc) / len(acc)
	#logresult(acc, total_vm)
	#print key
	#print data
	if (flag == 0):
		data[getkey(key)] = (acc, total_vm)
	elif (flag == 1):
		data3[getkey(key)] = (acc, total_vm)
	elif (flag == 2):
		data5[getkey(key)] = (acc, total_vm)
	print key, acc,total_vm



def getresult_9(infile, key, flag):
	global data

	total_vm = 0
	start = 0
	acc = []
	p = re.compile("accuracy = (\S*)")
	for line in open(infile):
		if "cmd" in line:
			total_vm += 1
		elif "start tg 9" in line:
			start = 1
		elif "stop tg 9" in line:
			start = 0
		else:
			m = p.match(line)
			if (m != None and start == 1):
				val = int(m.group(1))
				acc.append(val)
	#total = sum(acc)
	acc = sum(acc) / len(acc)
	#logresult(acc, total_vm)
	#print key
	#print data
	if (flag == 0):
		data[getkey(key)] = (acc, total_vm)
	elif (flag == 1):
		data3[getkey(key)] = (acc, total_vm)
	elif (flag == 2):
		data5[getkey(key)] = (acc, total_vm)
	print key, acc,total_vm
	#if key not in data:
	#	data[key] = [acc, total_vm]
	#if key not in data:
	#	data[key] = [acc, total_vm]


# 
# getresult("log_0_60_hs.txt", 0, 1)
# getresult("log_0.5_60_hs.txt", 500, 1)
# getresult("log_1_60_hs.txt", 1000, 1)
# getresult("log_1.5_60_hs.txt", 1500, 1)
# getresult("log_2_60_hs.txt", 2000, 1)
# getresult("log_2.5_60_hs.txt", 2500, 1)
# getresult("log_3_60_hs.txt", 3000, 1)
# getresult("log_3.5_60_hs.txt", 3500, 1)
# getresult("log_4_60_hs.txt", 4000, 1)
# 
# #exit(1)
# 
# 
# 
# 

getresult_9("log_60_9_1.txt", 9000, 0)
getresult_9("log_60_6_1.txt", 6000, 0)
getresult_4("log_60_4_1.txt", 4000, 0)
getresult_4("log_60_2_1.txt", 2000, 0)
getresult_4("log_60_1_1.txt", 1000, 0)
getresult_4("log_60_0_1.txt", 0, 0)

#exit(1)



getresult_9("log_120_9_1.txt", 9000, 1)
getresult_9("log_120_6_1.txt", 6000, 1)
getresult_4("log_120_4_1.txt", 4000, 1)
getresult_4("log_120_2_1.txt", 2000, 1)
getresult_4("log_120_1_1.txt", 1000, 1)
#getresult_4("log_120_0_1.txt", 0, 1)
data3[0] = data[0]
# 
# 
# 
# getresult("log_4_60_sb.txt", 4000, 2)
# 
# print "hehe ",data5
# exit(1)
# 

#getresult("log_4_60_hs.txt", 4000)


def getresult1(files, key, flag = "NULL"):
	global data

	overall_acc = []
	overall_vm = []
	for infile in files:
		total_vm = 0
		start = 0
		acc = []
		p = re.compile("accuracy = (\S*)")
		for line in open(infile):
			if "cmd" in line:
				total_vm += 1
			elif "start tg 4" in line:
				start = 1
			elif "stop tg 4" in line:
				start = 0
			else:
				m = p.match(line)
				if (m != None and start == 1):
					val = int(m.group(1))
					acc = val
		overall_acc.append(acc)
		overall_vm.append(total_vm)
	

	#print sum(overall_acc) / len(overall_acc)
	#print sum(overall_vm) / len(overall_vm)

	if flag == "sb":
		data2[getkey(key)] = (sum(overall_acc) / len(overall_acc), sum(overall_vm) / len(overall_vm))
	else:
		data1[getkey(key)] = (sum(overall_acc) / len(overall_acc), sum(overall_vm) / len(overall_vm))


getresult1(["log_10_1_1.txt", "log_10_1_2.txt", "log_10_1_3.txt", "log_10_1_4.txt","log_10_1_5.txt"], 1000)
getresult1(["log_10_2_1.txt", "log_10_2_2.txt", "log_10_2_3.txt", "log_10_2_4.txt","log_10_2_5.txt"], 2000)
getresult1(["log_10_4_1.txt", "log_10_4_2.txt", "log_10_4_3.txt", "log_10_4_4.txt","log_10_4_5.txt"], 4000)
getresult1(["log_10_6_1.txt", "log_10_6_2.txt", "log_10_6_3.txt", "log_10_6_4.txt","log_10_6_5.txt"], 6000)
getresult1(["log_10_9_1.txt", "log_10_9_2.txt", "log_10_9_3.txt", "log_10_9_4.txt","log_10_9_5.txt"], 9000)

getresult1(["log_10_1_sb_1.txt", "log_10_1_sb_2.txt"], 1000, "sb")
getresult1(["log_10_2_sb_1.txt", "log_10_2_sb_2.txt"], 2000, "sb")
getresult1(["log_10_4_sb_1.txt", "log_10_4_sb_2.txt"], 4000, "sb")
getresult1(["log_10_6_sb_1.txt", "log_10_6_sb_2.txt", "log_10_6_sb_3.txt"], 6000, "sb")
getresult1(["log_10_9_sb_1.txt", "log_10_9_sb_2.txt", "log_10_9_sb_3.txt"], 9000, "sb")


# getresult1("log_0_10_sb.txt", 0, "sb")
# getresult1("log_0.5_10_sb.txt", 500, "sb")
# getresult1("log_1_10_sb.txt", 1000, "sb")
# getresult1("log_1.5_10_sb.txt", 1500, "sb")
# getresult1("log_2_10_sb.txt", 2000, "sb")
# getresult1("log_2.5_10_sb.txt", 2500, "sb")


#print data1
data1[0] = data[0]
data3[0] = data[0]
data2[0] = data[0]

print "data ", data
print "data1 ", data1
print "data2 ", data2
print "data3 ", data3


def getresult2(infile, key):
	global data

	total_vm = 0
	start = 0
	acc = []
	p = re.compile("accuracy = (\S*)")
	for line in open(infile):
		if "cmd" in line:
			total_vm += 1
		elif "start tg 4" in line:
			start = 1
		elif "stop tg 4" in line:
			start = 0
		else:
			m = p.match(line)
			if (m != None and start == 1):
				val = int(m.group(1))
				acc = val
	#total = sum(acc)
	#acc = sum(acc) / len(acc)
	#
	#logresult(acc, total_vm)
	#print acc, total_vm
	data2[int(key/500)] = (acc, total_vm)
	#if key not in data1:
		#data1[key] = [acc, total_vm]

# 
# getresult2("log_0_10_hs.txt", 0)
# getresult2("log_0.5_10_hs.txt", 500)
# getresult2("log_1_10_hs.txt", 1000)
# getresult2("log_1.5_10_hs.txt", 1500)
# getresult2("log_2_10_hs.txt", 2000)
# getresult2("log_2.5_10_hs.txt", 2500)
# getresult2("log_3_10_hs.txt", 3000)
# getresult2("log_3.5_10_hs.txt", 3500)
# getresult2("log_4_10_hs.txt", 4000)
# 
# 
# print data2
# #print data2[0]
# #print len(data), len(data2)
# 
# 
# 
# getresult1("log_0_10_sb.txt", 0, "sb")
# getresult1("log_0.5_10_sb.txt", 500, "sb")
# getresult1("log_1_10_sb.txt", 1000, "sb")
# getresult1("log_1.5_10_sb.txt", 1500, "sb")
# getresult1("log_2_10_sb.txt", 2000, "sb")
# getresult1("log_2.5_10_sb.txt", 2500, "sb")
# getresult1("log_3_10_sb.txt", 3000, "sb")
# getresult1("log_3.5_10_sb.txt", 3500, "sb")
# getresult1("log_4_10_sb.txt", 4000, "sb")
# 
# 
print data1
for ii in range(len(data)):
	#print ii
	total_log.write(str(getval(ii))	+ '\t' 
			+ str(data1[ii][0]) + '\t' + str(data1[ii][1]) + '\t'
			+ str(data[ii][0]) + '\t' + str(data[ii][1]) + '\t' 
			+ str(data3[ii][0]) + '\t' + str(data3[ii][1])  + '\t'
			+ str(data2[ii][0]) + '\t' + str(data2[ii][1]) + 
			'\r\n')

#for ii in range(len(data)):
#	print ii
#	total_log.write(str(ii * 500)+ '\t' +
#			+ str(data1[ii][0]) + '\t' + str(data1[ii][1]) +
#			'\r\n')







print data4

#print data
#for key in data:
	#print key
	#print data[key][1]
	#exit(1)

	#logresult(acc, total_vm)
	#total_log.write(key +" " + str(data[key][0]) + '\t' + str(data[key][1]) + str(data1[key][0]) + '\t' + str(data1[key][1])+ '\r\n')
