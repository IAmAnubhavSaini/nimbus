import re

total_log = open("count_total.plo","w")

data = [[] for ii in range(6)]   #60 sec 1%
data1 = [[] for ii in range(6)] # 10 sec 1%
data2 = [[] for ii in range(6)] # 10 sec 1% standby
data3 = [[] for ii in range(6)] # 60 sec 10%

data4 = [[] for ii in range(6)] # 10 sec 1% standby
data5 = [[] for ii in range(6)] # 60 sec 1% standby


def getkey(val):
	array = [0, 1000, 2000, 4000, 6000, 9000]
	for ii in range(len(array)):
		if array[ii] == val:
			return ii
	return -1


def logresult(acc, total_vm):
	global total_log
	#print acc
	total_log.write(str(acc) + '\t' + str(total_vm) + '\r\n')

def getresult(infile, key, flag):
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
# getresult("log_0_60.txt", 0, 0)
# getresult("log_0.5_60.txt", 500, 0)
# getresult("log_1_60.txt", 1000, 0)
# getresult("log_1.5_60.txt", 1500, 0)
# getresult("log_2_60.txt", 2000, 0)
# getresult("log_2.5_60.txt", 2500, 0)
# getresult("log_3_60.txt", 3000, 0)
# getresult("log_3.5_60.txt", 3500, 0)
# getresult("log_4_60.txt", 4000, 0)
# 
# 
# 
# 
# getresult("log_4_60_sb.txt", 4000, 2)
# 
# print "hehe ",data5
# exit(1)
# 

#getresult("log_4_60_hs.txt", 4000)


def getresult1(infile, key, flag = "NULL"):
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
	if flag == "sb":
		data4[getkey(key)] = (acc, total_vm)
	else:
		data1[getkey(key)] = (acc, total_vm)
	#if key not in data1:
		#data1[key] = [acc, total_vm]


#getresult("log_0_10.txt", 0)
#getresult1("log_0.5_10.txt", 500)
getresult1("log_1_50_10_1.txt", 1000)
#getresult1("log_1.5_10.txt", 1500)
getresult1("log_1_50_10_2.txt", 2000)
#getresult1("log_2.5_10.txt", 2500)
#getresult1("log_3_10.txt", 3000)
#getresult1("log_3.5_10.txt", 3500)
getresult1("log_1_50_10_4.txt", 4000)

print data1
data1[0] = data[0]
#data2[0] = data[0]






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
#print data1
#for ii in range(len(data)):
	#print ii
#	total_log.write(str(ii * 500)+ '\t' + str(data[ii][0]) + '\t' + str(data[ii][1]) + '\t' 
#			+ str(data1[ii][0]) + '\t' + str(data1[ii][1]) + '\t'
#			+ str(data3[ii][0]) + '\t' + str(data3[ii][1]) + '\t'
#			+ str(data2[ii][0]) + '\t' + str(data2[ii][1]) + '\t'
#			+ str(data4[ii][0]) + '\t' + str(data4[ii][1]) + '\t'
#			'\r\n')

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
