import networkx as nx
import operator
import sys
import os
from prettytable import PrettyTable

filenet = open(sys.argv[1]+'/waitfor','r')
filetgroup = open(sys.argv[1]+'/merge.tinfo','r')
filesoftirq = open(sys.argv[1]+'/soft.tinfo','r')
fileallthreads = open(sys.argv[1]+'/target.tinfo','r')
filebreak = open(sys.argv[1]+'/breakdown','r')
filekmesg = open(sys.argv[1]+'/kmesg','r')
filefs = open(sys.argv[1]+'/gresult','r')
fileperf = open(sys.argv[1]+'/presult.out','r')
filecpu = open(sys.argv[1]+'/cpufreq','r')

tdir = sys.argv[1]
tlang = sys.argv[2]
term = float(sys.argv[3])
removeThreshold = float(sys.argv[4])

btable = None
btag = 0
tmprank = {}

softirq=[]
allthreads=[]
#Create softirq group
for l in filesoftirq:
	l=l.split()
	for e in l:
		softirq.append(int(e))

for l in fileallthreads:
    l = l.split()
    for e in l:
        if int(e) not in softirq:
            allthreads.append(int(e))

#'''
for l in filebreak:
    l=l.split()
    if btag == 0:
        btable = PrettyTable(l)
        btag = 1
    else:
        total = 0.0
        tmp = []
        # Don't show softirq
        #if int(l[0]) in softirq:
        #    continue
        if float(l[-1]) == 0:
            for e in l[1:-1]:
                tmp.append('-');
            continue
        for e in l[1:]:
            total+=float(e) 
        tmp.append(l[0])
        for e in l[1:-1]:
            tmp.append(format(float(e)/float(l[-1])*100,'.2f')+'%')
        tmp.append(l[-1])
        tmprank[int(l[0])]=tmp
for k in sorted(tmprank.keys()):
    btable.add_row(tmprank[k])
#'''

# For memcached
#netrate = 0.436
#diskrate = 0

# For mysql memory 8G
#netrate = 34.4/94.3
# For mysql memory 128M
#netrate = 25.5/94.3
#netrate = 0.01
#diskrate = 0.934

# For mysql disk
#netrate = 0.01
#diskrate = 0.98
stime = 0.0
etime = 0.0
dtime = 0.0
ntime = 0.0

freq = 0
for l in filecpu:
	freq = float(l.strip())*1e9

send = {}
sendTotal = 0
for k in filekmesg:
    l = k[0]+'00'+k[3:]
    if 'Inside open' in l:
        stime = float(l.split()[-1])/freq
    elif 'Inside close' in l:
        etime = float(l.split()[-1])/freq
    elif 'Disk' in l and 'work time' in l:
        dtime = float(l.split()[-1])/1000.0
    elif 'Network:' in l:
        ntime = float(l.split()[-1])/1000.0
    elif 'sends' in l:
        tmp = l.strip().split()
        pid = int(tmp[2])
        byte = int(tmp[-1])
        send[pid] = byte 
        sendTotal += byte
    
totaltime = etime - stime
#diskrate = dtime / totaltime 
#netrate = sendTotal / (125000000 * totaltime)
threshold = totaltime * removeThreshold
#threshold = (etime - stime) * 0.005

tgroup={}
nodes=[]
originG={}
startG={}
finalG=[]
G=nx.DiGraph()

#Create a thread group
index = 0
for l in filetgroup:
    l=l.split()
    tgroup[int(l[0])] = []
    for e in l:
        tgroup[int(l[0])].append(int(e))

javaBack=[]

threadName = {}

def tName(tid):
    if tid in threadName.keys():
        return threadName[tid]
        #return threadName[tid].split(':')[0].split('.')[0].split('(')[0]
    return tid

fs={}
wt={}
wk={}

pid = 0
pid1 = 0
fstype = 0
fstr = ''
for l in fileperf:
	if l == '\n':
		continue
	if 'pid=' in l or 'sched:sched_switch'in l:
		if pid != 0:
			if fstype == 0:
				for gkey in tgroup.keys():
					if pid in tgroup[gkey]:
						pid = gkey
						break
				if pid not in wt.keys():
					wt[pid] = {}
				if fstr not in wt[pid].keys():
					wt[pid][fstr] = 0
				wt[pid][fstr] += 1
			else:
				for gkey in tgroup.keys():
					if pid in tgroup[gkey]:
						pid = gkey
						break

				for gkey in tgroup.keys():
					if pid1 in tgroup[gkey]:
						pid1 = gkey
						break
	
				if (pid,pid1) not in wk.keys():
					wk[(pid,pid1)] = {}
				if fstr not in wk[(pid,pid1)].keys():
					wk[(pid,pid1)][fstr] = 0
				wk[(pid,pid1)][fstr] += 1
		pid = int(l.split()[1])
		if 'sched_switch' in l:
			fstype = 0
		else:
			fstype = 1
			pid1 = int(l.split('pid=')[1].split()[0])
		fstr = ''
	else:
		fstr += l.strip()+'\n'

# Get the thread name 
fstr = ''
tid = 0
if tlang == 'java':
	for l in filefs:
		if '(Parallel GC Threads)' in l or '(ParallelGC)' in l:
			tid = int(l.split('nid=')[1].split()[0])
			threadName[str(tid)] = 'ParallelGC'
		elif '(Parallel CMS Threads)' in l:
			tid = int(l.split('nid=')[1].split()[0])
			threadName[str(tid)] = 'ParallelCMS'
		elif 'VM Thread' in l:
			tid = int(l.split('nid=')[1].split()[0])
			threadName[str(tid)] = 'VMThread'
		elif 'at ' in l or 'nid=' in l:
			if 'nid=' in l:
				if tid != 0:
					for gkey in tgroup.keys():
						if tid in tgroup[gkey]:
							tid = gkey
							break
					if tid not in fs.keys():
						fs[tid] = {}
					if fstr not in fs[tid].keys():
						fs[tid][fstr] = 0
					fs[tid][fstr] += 1
				tid = int(l.split('nid=')[1].split()[0])
				fstr = ''
				nameList = l.split('"')[1].replace('"','').split()
				name = ''
				for tmp in nameList:
				    name += tmp + ' '
				    if tmp.isdigit():
				        break
				threadName[str(tid)] = name + '(' + str(tid)+')'
			elif 'at ' in l:
				fstr += l.strip() + '\n'
else:
	pid = 0
	fstr = ''
	laststr=''
	thisstr=''
	for l in filefs: 
	        if '#' in l:
		    laststr = thisstr
		    thisstr = l
		if 'start_thread' in l:
		    if '0x' in laststr:
		        threadName[str(pid)] = laststr.split()[3] + '(' + str(pid) + ')'
		    else:
		        threadName[str(pid)] = laststr.split()[1] + '(' + str(pid) + ')'
		if 'Total stack' in l or 'Thread: ' in l:
			if pid != 0:
				if pid not in fs.keys():
					fs[pid] = {}
				if fstr not in fs[pid].keys():
					fs[pid][fstr] = 0
				fs[pid][fstr] += 1
			if 'Total stack' in l:
				break
			pid = int(l.split()[1])
			for gkey in tgroup.keys():
				if pid in tgroup[gkey]:
					pid = gkey
					break
			fstr = ''
		else:
			fstr += l.strip()+'\n'
#print threadName

#Create the originG network

for l in filenet:
    l=l.split()
    nodeA=int(l[0])
    nodeB=int(l[1])
    edge=float(format(float(l[2]),'.2f'))
    # No need for unkown(-99), scheduler(0), softirq, hardirq/timer, and softirq kernel thread
    #if (nodeB==-99) or (nodeB==0):
    if (nodeB==-99) or (nodeB==0) or (nodeB==-16) or (nodeB==-15) or (nodeB == -2) or (nodeA in softirq):
    	continue
    startG[(nodeA,nodeB)]=edge

    for gkey in tgroup.keys():
        if nodeA in tgroup[gkey]:
            nodeA = gkey
            break
    for gkey in tgroup.keys():
        if nodeB in tgroup[gkey]:
            nodeB = gkey
            break
    if nodeA not in nodes:
        nodes.append(nodeA)
    if nodeB not in nodes:
        nodes.append(nodeB)

#Add edge from network to device
netlist={}
total = 0.0

for nodeA in nodes:
    for nodeB in nodes:
        if nodeA == nodeB:
            continue
        if (nodeA, nodeB) not in startG.keys():
            continue
        if nodeA in tgroup.keys():
            group1 = tgroup[nodeA]
        else:
            group1 = [nodeA]
        if nodeB in tgroup.keys():
            group2 = tgroup[nodeB]
        else:
            group2 = [nodeB]

        weight = 0.0
        for m in group2:
            tmpweight = 0.0
            count = 0
            for n in group1:
                if (n,m) in startG.keys():
                    #count+=1
                    tmpweight+=startG[(n,m)]
            weight+=tmpweight/len(group1)
        if weight != 0.0:
            originG[(nodeA, nodeB)] = weight;

        #if nodeA == 27196 and nodeB == 27198:
        #    print nodeA, nodeB, weight
        #    raw_input()


for k in originG.keys():
	# Filter!!! Weight should more than 3 second
	if originG[k] < threshold:
		continue
	finalG.append((k[0],k[1],originG[k]));

def findName(tlist):
	res=''
	if type(tlist) is list:
		for value in tlist:
			if value == -4:
				res+='NIC, '
			elif value == -5:
				res+='Disk, '
			elif value == -2:
			        res+='Timer, '
			elif value == -15:
				res+='HardIRQ(Timer), '
			elif value == -99:
				res+='Unknown, '
			else:
			        res+=str(tName(str(value)))+', '
				#res+=str(value)+', '
		return res[:-2]
	else:
		value = tlist
		if value == -4:
			res+='NIC'
		elif value == -5:
			res+='Disk'
		elif value == -2:
	        	res+='Timer'
		elif value == -15:
			res+='HardIRQ(Timer)'
		elif value == -99:
			res+='Unknown'
		else:
		        res+=str(tName(str(value)))
		#	res+=str(value)
		return res

def scc_graph(G):
	scc_result = []
	result=[list(c) for c in sorted(nx.strongly_connected_components(G), key=len, reverse=True)]
	for i in range(0, len(result)):
	    waitfor = ''
	    outgoing = 0.0
	    for j in range(0, len(result)):
	    	if i==j:
	    		continue
	    	for m in range(0, len(result[i])):
	    		for n in range(0, len(result[j])):
	    			if G.has_edge(result[i][m], result[j][n]):
	    				outgoing += G.get_edge_data(result[i][m], result[j][n])['weight']
	    if outgoing == 0:
	    	memo = 'Bottleneck'
	    else:
	        memo = 'Normal SCC'

	    for j in range(0, len(result)):
	        weight = 0.0
                if i == j:
                    continue
                for m in range(0, len(result[i])):
                    for n in range(0, len(result[j])):
                        if G.has_edge(result[j][n],result[i][m]):
                            weight += G.get_edge_data(result[j][n],result[i][m])['weight']
                if weight != 0:
                    waitfor+=str(j)+'->'+str(weight)+'|'
	    #waitfor+='SCC '+str(j)+':'+str(tmpweight)+'|'
	    scc_result.append((result[i], weight, memo, waitfor[:-1]))
	return scc_result

#print result

def show_weight(vlist):
	wresult={}
	for i in range(0, len(vlist)):
		for j in (range(0, len(vlist))):
			if i==j:
				continue
			else:
				for e in finalG:
					if e[0] == vlist[i] and e[1] == vlist[j]:
						wresult[(e[0],e[1])] = e[2]
	wkey = sorted(wresult, key=wresult.get, reverse=True)
	res={}
	for i in range(0,len(wkey)):
		res[(wkey[i][0],wkey[i][1])] = wresult[wkey[i]]
	return res 

def addColor(string):
	return '\x1b[6;31;40m'+string+'\x1b[0m'

def print_scc(scc_result):
	#print 'SCC Components:'
	x = PrettyTable(["SCC ID", "SCC Members", "Type", "Intra-Wait", "Inter-Wait", "NumOfEdges", "CPU Usage"])
	#x = PrettyTable(["SCC ID", "SCC Members", "Type", "Intra-Wait", "Inter-Wait"])
	x.max_width['SCC Members']=40
	#print 'SCC Components:'
	#print '%s\t%s\t%s\t%s' % ('Number'.center(5),'Vertex Group'.center(40),'Weight'.center(10), 'Memo'.center(10))
	for i in range(0,len(scc_result)):
		intraWait = 0.0
		if len(scc_result[i][0])==1 and scc_result[i][0][0] in tgroup.keys():
		    tmpnode = scc_result[i][0][0]
		    for m in tgroup[tmpnode]:
		        for n in tgroup[tmpnode]:
		            if m == n:
		                continue
		            else:
		                if (m,n) in startG.keys():
                                    intraWait+=startG[(m,n)]
                    intraWait /= (len(tgroup[scc_result[i][0][0]]))
		else:
		    numEdge = 0
		    for m in scc_result[i][0]:
		        for n in scc_result[i][0]:
		            if m==n:
		                continue
		            else:
		                for k in finalG:
                                    if k[0]==m and k[1]==n:
                                        intraWait+=k[2]
                                        break

                for spid in scc_result[i][0]:
                    for epid in scc_result[i][0]:
                        if G.has_edge(spid,epid):
                            numEdge += 1

                cpuutil = 0.0

                #print tmprank
                for spid in scc_result[i][0]:
                    if spid in tmprank.keys():
                        cpuutil += float(tmprank[spid][1].replace('%',''))
                cpuutil=str(cpuutil)+'%'

                thstr = findName(scc_result[i][0])
                if len(thstr.split(',')) > 5:
                    thtmp = ''
                    for k in thstr.split(',')[:5]:
                        thtmp+=k+','
                    thtmp+='...'
                    thstr = thtmp
		if scc_result[i][2]=='Bottleneck':
			#x.add_row([addColor(str(i)), addColor(findName(scc_result[i][0])),addColor(str(scc_result[i][1])), addColor(scc_result[i][2]), addColor(str(intraWait)), addColor(str(scc_result[i][3]))])
			x.add_row([addColor(str(i)), addColor(thstr), addColor(scc_result[i][2]), addColor(str(intraWait)), addColor(str(scc_result[i][3])), numEdge, cpuutil])
		else:
			#x.add_row([str(i), findName(scc_result[i][0]),str(scc_result[i][1]), scc_result[i][2], intraWait, str(scc_result[i][3])])
			x.add_row([str(i), findName(thstr), scc_result[i][2], intraWait, str(scc_result[i][3]), numEdge, cpuutil])
		#print '%s\t%s\t%s\t%s' % (str(i).center(5), str(scc_result[i][0]).center(40), str(scc_result[i][1]).center(10), scc_result[i][2].center(10))
	j = len(scc_result)
	if (len(previous)==0):
	    for n in allthreads:
	        ifshow=1
	        for i in range(0,len(scc_result)): 
	            for m in scc_result[i][0]:
	                if m in tgroup.keys():
	                    if n in tgroup[m]:
	                        ifshow = 0
	                        break
	                else:
	                    if n == m:
	                        ifshow = 0
	                        break
	        if ifshow == 1:
	            for thead in tgroup.keys():
	                if int(n) == thead:
	                    break
	                if int(n) in tgroup[thead]:
	                    ifshow = 0
	                    break
	        #if ifshow==1:
                #    x.add_row([str(j), n, 'Normal SCC', 0, ''])
                #    j+=1
	print x

def create_new(vlist, G, op):
	new_G=nx.DiGraph()
	new_list = []
	for i in range(0,len(vlist)):
		for j in range(0,len(vlist)):
			if i==j:
				continue
			else:
				for e in finalG:
					if G.has_edge(e[0], e[1]) and e[0] == vlist[i] and e[1] == vlist[j]:
						new_list.append((e[0],e[1],e[2]))

	tmpmin = sys.float_info.max
	if (op == 0):
		for i in new_list:
			if i[2] < tmpmin:
				tmpmin = i[2]
		if tmpmin > term:
			print '\x1b[6;31;40m'+'[Warn] Termination condition is met, you can stop here.'
		for i in range(0,len(new_list)):
			e = new_list[i]
			if e[2] == tmpmin:
				del new_list[i]
				print '\x1b[6;31;40m'+'[INFO] Remove edge from %s to %s with weight %f' % (findName(e[0]), findName(e[1]), e[2]) + '\x1b[0m'	
				break
	new_G.add_weighted_edges_from(new_list)
	return new_G

#Create the network based on originG
G.add_weighted_edges_from(finalG)

previous=[]

while(1):
    try:
    	scc_result = scc_graph(G)
	if len(scc_result) == 1 and len(scc_result[0])!=1:
                print '\x1b[6;31;40m[INFO] Only single SCC in current graph. Need to remove the shortest weights in the graph until there\'re more than one SCC.\x1b[0m'
                #print '\x1b[6;31;40m[INFO] The current graph info is shown below.\x1b[0m'
                #einfo={}
                #x = PrettyTable(["Vertex A", "Vertex B", "Weight"])
                #for i in scc_result[0][0]:
                #    for j in scc_result[0][0]:
                #        if i==j:
                #            continue
                #        if G.has_edge(i,j):
                #            einfo[(i,j)] = G.get_edge_data(i,j)['weight']
                #            #x.add_row([findName(i), findName(j), str(G.get_edge_data(i,j)['weight'])])
                #einfo=sorted(einfo.items(), key=operator.itemgetter(1), reverse=True)
		#for e in einfo:
		#    x.add_row([findName(e[0][0]),findName(e[0][1]),str(e[1])])
                #print x
                print '\x1b[6;31;40m[INFO] Press any key to continue...\x1b[0m',
                raw_input()
		G = create_new(scc_result[0][0], G, 0)
		continue
	print 'Termination condition: ', term*totaltime
	print_scc(scc_result);
	keypress = raw_input('Command (h for help):')
	#keypress = sys.stdin.readline().strip()
	#keypress = keypress.replace('\n','').replace('\r','')
	if keypress=='q':
	    os._exit(1)
	elif 'g' in keypress[0] and 'w' in keypress[1]:
		pos=int(keypress.split()[1])
		tmpList={}

		if pos == -1:
		    f1=open('waitfor.csv','w')
		    for k in finalG:
		        f1.write('%s,%s,%s\n' % (k[0],k[1],k[2]))
		    f1.close()
		elif len(scc_result[pos][0]) == 1:
			if scc_result[pos][0][0] not in tgroup.keys():
				print '\x1b[6;31;40m'+'[ERROR] Cannot unfold this SCC any more'+'\x1b[0m'
			else:
                                einfo={}
				tmpid = scc_result[pos][0][0] 
                                x = 'source,target,value\n'
				for m in tgroup[tmpid]:
					for n in tgroup[tmpid]:
						if (m,n) in startG.keys():
                                                    einfo[(m,n)] = str(startG[(m,n)])
                                                    #x.add_row([findName(m),findName(n),str(startG[(m,n)])])
                                einfo=sorted(einfo.items(), key=operator.itemgetter(1), reverse=True)
                                for e in einfo:
                                    x += '%s,%s,%.2f\n' % (tName(findName(e[0][0]))+'-'+str(tgroup[tmpid].index(e[0][0])),tName(findName(e[0][1]))+'-'+str(tgroup[tmpid].index(e[0][1])),e[1])
				print x
                else:
                    einfo={}
	    	    for m in scc_result[pos][0]:
	    	    	for n in scc_result[pos][0]:
	    	    		if G.has_edge(m,n):
	    	    			einfo[(m,n)] = G.get_edge_data(m,n)['weight']
	    	    for tmpid in scc_result[pos][0]:
	    	        tmpsize=0
	    	        if tmpid not in tgroup.keys():
	    	            continue
	    	        for m in tgroup[tmpid]:
	    	            for n in tgroup[tmpid]:
	    	                if (m,n) in startG.keys():
	    	                    tmpsize+=startG[(m,n)]
	    	        einfo[(tmpid,tmpid)] = tmpsize
	    	    for tpos in range(0,len(scc_result)):
	    	        if tpos == pos:
	    	            continue
	    	        for m in scc_result[tpos][0]:
	    	            for n in scc_result[pos][0]:
	    	                if G.has_edge(m,n):
	    	    		    einfo[(m,n)] = G.get_edge_data(m,n)['weight']
	    	    einfo=sorted(einfo.items(), key=operator.itemgetter(1), reverse=True)
	    	    x = 'source,target,value\n'
	    	    for e in einfo:
	    	        #x += '%s,%s,%s\n' % (findName(e[0][0]),findName(e[0][1]),str(e[1]))
	    	        x += '%s,%s,%.2f\n' % (tName(findName(e[0][0])),tName(findName(e[0][1])),e[1])
	    	    f1=open('waitfor.csv','w')
	    	    f1.write(x)
	    	    f1.close()
	    	    print x
	    	raw_input('Press any key to continue...')
	elif 'g' in keypress[0]:
		pos=int(keypress.split()[1])
		tmpList={}

		if pos == -1:
		    f1=open('waitfor.csv','w')
		    for k in finalG:
		        f1.write('%s,%s,%s\n' % (k[0],k[1],k[2]))
		    f1.close()
		elif len(scc_result[pos][0]) == 1:
			if scc_result[pos][0][0] not in tgroup.keys():
				print '\x1b[6;31;40m'+'[ERROR] Cannot unfold this SCC any more'+'\x1b[0m'
			else:
                                einfo={}
				tmpid = scc_result[pos][0][0] 
                                x = 'source,target,value\n'
				for m in tgroup[tmpid]:
					for n in tgroup[tmpid]:
						if (m,n) in startG.keys():
                                                    einfo[(m,n)] = str(startG[(m,n)])
                                                    #x.add_row([findName(m),findName(n),str(startG[(m,n)])])
                                einfo=sorted(einfo.items(), key=operator.itemgetter(1), reverse=True)
                                for e in einfo:
                                    x += '%s,%s,%.2f\n' % (tName(findName(e[0][0]))+'-'+str(tgroup[tmpid].index(e[0][0])),tName(findName(e[0][1]))+'-'+str(tgroup[tmpid].index(e[0][1])),e[1])
				print x
                else:
                    einfo={}
	    	    for m in scc_result[pos][0]:
	    	    	for n in scc_result[pos][0]:
	    	    		if G.has_edge(m,n):
	    	    			einfo[(m,n)] = G.get_edge_data(m,n)['weight']
	    	    for tmpid in scc_result[pos][0]:
	    	        tmpsize=0
	    	        if tmpid not in tgroup.keys():
	    	            continue
	    	        for m in tgroup[tmpid]:
	    	            for n in tgroup[tmpid]:
	    	                if (m,n) in startG.keys():
	    	                    tmpsize+=startG[(m,n)]
	    	        einfo[(tmpid,tmpid)] = tmpsize
	    	    einfo=sorted(einfo.items(), key=operator.itemgetter(1), reverse=True)
	    	    x = 'source,target,value\n'
	    	    for e in einfo:
	    	        #x += '%s,%s,%s\n' % (findName(e[0][0]),findName(e[0][1]),str(e[1]))
	    	        x += '%s,%s,%.2f\n' % (tName(findName(e[0][0])),tName(findName(e[0][1])),e[1])
	    	    f1=open('waitfor.csv','w')
	    	    f1.write(x)
	    	    f1.close()
	    	    print x
	    	raw_input('Press any key to continue...')
	elif 's' in keypress[0]:
		pos=int(keypress.split()[1])
		tmpList={}
		if len(scc_result[pos][0]) == 1:
			if scc_result[pos][0][0] not in tgroup.keys():
				print '\x1b[6;31;40m'+'[ERROR] Cannot unfold this SCC any more'+'\x1b[0m'
			else:
                                einfo={}
				tmpid = scc_result[pos][0][0] 
				x = PrettyTable(["Vertex A", "Vertex B", "Weight"])
				for m in tgroup[tmpid]:
					for n in tgroup[tmpid]:
						if (m,n) in startG.keys():
                                                    einfo[(m,n)] = str(startG[(m,n)])
                                                    #x.add_row([findName(m),findName(n),str(startG[(m,n)])])

                                einfo=sorted(einfo.items(), key=operator.itemgetter(1), reverse=True)
                                for e in einfo:
                                    x.add_row([findName(e[0][0]),findName(e[0][1]),str(e[1])])
				print x
		else:
			einfo={}
			for m in scc_result[pos][0]:
				for n in scc_result[pos][0]:
					if G.has_edge(m,n):
						einfo[(m,n)] = G.get_edge_data(m,n)['weight']
			einfo=sorted(einfo.items(), key=operator.itemgetter(1), reverse=True)
			print 'Sub-grpaph Information:'
			x = PrettyTable(["Vertex A", "Vertex B", "Weight"])
			for e in einfo:
				x.add_row([findName(e[0][0]),findName(e[0][1]),str(e[1])])
			print x
	elif keypress=='a':
	    print 'All Edges Information:'
	    x = PrettyTable(["Vertex A", "Vertex B", "Weight"])
	    for i in range(0,len(finalG)):
	        #if finalG[i][0] in javaBack or finalG[i][1] in javaBack:
	        #    continue
	    	x.add_row([findName(finalG[i][0]), findName(finalG[i][1]), str(finalG[i][2])])
	    print x
	elif keypress=='b':
	    if (len(previous) == 0):
	        print '\x1b[6;31;40m'+'[ERROR] Cannot get back any more'+'\x1b[0m'
	    else:
	        G = previous.pop()
	elif keypress=='o':
	    print btable
	elif 'edge' in keypress:    
	    pid1 = int(keypress.split()[1])
	    pid2 = int(keypress.split()[2])
	    for i in scc_result[pid1][0]:
	        for j in scc_result[pid2][0]:
	            #print i,j
	            for k in finalG:
	                if k[0] == i and k[1] == j:
	                    print k[0],k[1],k[2]
	    #print scc_result[pid1]
	    #print scc_result[pid2]
	elif 'wt' in keypress:
		pid1 = int(keypress.split()[1])
		if len(keypress.split()) == 2:
		    size = 3
		else:
		    size = int(keypress.split()[2])
		total = 0
		for l in wt[pid1].values():
			total+=l
		x = PrettyTable(["TID", "Function Stack", "NumOfEvents", "Percent"])
		x.align['Function Stack']='l'
		tmplist = sorted(wt[pid1], key=wt[pid1].get, reverse=True)
		tmpi = 0
		for k in tmplist:
		    print str(pid1), k[:-1], str(wt[pid1][k]), format(wt[pid1][k]*100.0/total,'2.2f')+'%'
		    x.add_row([str(pid1), k[:-1], str(wt[pid1][k]), format(wt[pid1][k]*100.0/total,'2.2f')+'%'])
		    tmpi+=1
		    if tmpi == size:
		        break
		print x
		raw_input('Press any key to continue...')
	elif 'wk' in keypress:
	    pid1 = int(keypress.split()[1])
	    pid2 = int(keypress.split()[2])
	    if len(keypress.split()) == 3:
	        size = 3
	    else:
	        size = int(keypress.split()[3])
	    total = 0
	    for l in wk[(pid1,pid2)].values():
	        total+=l
            x = PrettyTable(["TID1", "TID2", "Function Stack", "NumOfEvents", "Percent"])
            x.align['Function Stack']='l'
            tmplist = sorted(wk[(pid1,pid2)], key=wk[(pid1,pid2)].get, reverse=True)
            tmpi = 0
            for k in tmplist:
                x.add_row([str(pid1), str(pid2), k[:-1], str(wk[(pid1,pid2)][k]), format(wk[(pid1,pid2)][k]*100.0/total,'2.2f')+'%'])
                tmpi+=1
                if tmpi == size:
                    break
            print x
            raw_input('Press any key to continue...')
	#elif keypress == 'u':
	#	x = PrettyTable(["Disk Utilization", "Network Utilization"])
	#	x.add_row([str(diskrate*100)+'%', str(netrate*100)+'%'])
	#	print x
	elif 'fs' in keypress.split()[0]:
	    pid = int(keypress.split()[1])
	    if len(keypress.split()) == 2:
	        size = 3
	    else:
	        size = int(keypress.split()[2])
	    total = 0
	    for l in fs[pid].values():
	        total+=l
            x = PrettyTable(["Thread ID", "Function Stack", "Percent"])
            x.align['Function Stack']='l'
            tmplist = sorted(fs[pid], key=fs[pid].get, reverse=True)
            tmpi = 0
            for k in tmplist:
                x.add_row([str(pid), k[:-1], format(fs[pid][k]*100.0/total,'2.2f')+'%'])
                tmpi+=1
                if tmpi == size:
                    break
            print x
            raw_input('Press any key to continue...')
	elif int(keypress) >= 0 and int(keypress) < len(scc_result):
		if len(scc_result[int(keypress)][0]) == 1:
			print '\x1b[6;31;40m'+'[ERROR] Cannot unfold this SCC any more'+'\x1b[0m'
		else:
			previous.append(G);
			G = create_new(scc_result[int(keypress)][0], G, 1)
			#sw = show_weight(scc_result[int(keypress)][0]);		
			#print '%s\t%s\t%s' % ('Starting point'.center(10),'End point'.center(10),'Weight'.center(10))
			#for k in sw.keys():
			#	print '%s\t%s\t%s' % (str(k[0]).center(10),str(k[1]).center(10),str(sw[k]).center(10))
	else:
		print '\x1b[6;31;40m'+'[ERROR] Wrong command'+'\x1b[0m'
		#print 'Wrong command!'
    except:
        raw_input('\x1b[6;31;40m'+'Exception! Press any key to continue...'+'\x1b[0m')
        os.system('clear')  # For Linux/OS X
