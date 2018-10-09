import sys
import networkx as nx

#Current assume netwrok bandwidth: 125MB/s
nband = 125000000

class node:
    parent = 0
    pid = 0
    stime = 0
    etime = 0
    
    def printOut(self):
        print self.pid, self.parent, self.stime, self.etime

cpuFreq = 0
fcpu = open('cpufreq','r')
for l in fcpu:
    cpuFreq = float(l.strip())*1e9
fcpu.close()

f=open('kmesg')

nodes = {}

starttime = 0
endtime = 0

dtime = 0.0
ntime = 0.0

send = {}
sendTotal = 0

for l in f:
    l = l[0] + '0000' + l[5:]
    if 'Create' in l:
        tmpstr = l.split('Thread')[1].split(' ')
        pid1 = int(tmpstr[1])
        pid2 = int(tmpstr[4])
        stime = int(tmpstr[-1])
        nodes[pid2] = node()
        nodes[pid2].pid = pid2
        nodes[pid2].parent = pid1
        nodes[pid2].stime = stime
    elif 'Finish' in l:
        tmpstr = l.split('Thread')[1].split(' ')
        pid1 = int(tmpstr[1])
        etime = int(tmpstr[-1])
        if pid1 not in nodes.keys():
            continue
        else:
            nodes[pid1].etime = etime
    elif 'Inside open' in l:
        starttime = int(l.split()[-1])
    elif 'Inside close' in l:
        endtime = int(l.split()[-1])
    elif 'Disk work time' in l:
        dtime = float(l.split()[-1])/1000.0
    elif 'sends' in l:
        tmp = l.strip().split()
        pid = int(tmp[2])
        byte = int(tmp[-1])
        send[pid] = byte 
        sendTotal += byte

# Network backtrace can be done here.
# Disk backtrace should still be done in scc-pypy. Because the information still in the original waitfor file.
totaltime = (endtime - starttime)/cpuFreq
netrest = (1 - sendTotal/(nband * totaltime)) * totaltime
diskrest = (1 - dtime/totaltime) * totaltime

#parent and child's set
pcset = {}

for node in nodes.values():
    if node.stime == 0 or node.etime == 0:
        continue
    if node.parent not in pcset.keys():
        pcset[node.parent] = []
    pcset[node.parent].append(node.pid)

#print pcset
#parent and child's final set
pcfset = {}
for k in pcset.keys():
    edges = []
    if len(pcset[k]) == 1:
        pcfset[k] = []
        pcfset[k].append(list(pcset[k]))
        continue
    G = nx.Graph()
    for i in pcset[k]:
        for j in pcset[k]:
            if i == j:
                continue
            if nodes[i].stime > nodes[j].stime and nodes[i].stime < nodes[j].etime - 100000000:
                edges.append((i,j))
    if len(edges) > 0:
        G.add_edges_from(edges)
        result=sorted(nx.connected_components(G), key=len, reverse=True)
        pcfset[k] = []
        if (len(result) >= 1):
            for m in result:
                pcfset[k].append(list(sorted(m)))
    else:
        pcfset[k] = []
        for kk in sorted(pcset[k]):
            tlist = []
            tlist.append(kk)
            pcfset[k].append(tlist)

#long_term threads
lt_thread = []

for k in sorted(pcfset.keys()):
    islt = 0
    for ltotal in pcfset.values():
        for l in ltotal:
            if k in l:
                islt = 1
                break
    if islt == 0:
        lt_thread.append(k)
    #print k, pcfset[k]

#print lt_thread

merge={}
mresult={}

def recursive_merge(k):
    maxlen = 0
    merge[k] = []
    for v in pcfset[k]:
        maxlen = len(v) if len(v) > maxlen else maxlen
    for i in range(0,maxlen):
        merge[k].append([])
    for i in pcfset[k]:
        for j in range(0, maxlen):
            if len(i) < j+1:
                continue
            else:
                merge[k][j].append(i[j])
    mresult[k] = []
    lenset = 0
    for i in range(0,maxlen):
        lenset = len(merge[k][i]) if merge[k][i] > lenset else lenset
    for i in range(0,lenset):
        mresult[k].append([])
    for v in merge[k]:
        for i in range(0,lenset):
            mresult[k][i].append(v[i])
    # only consider one level
    if v[0] not in pcfset.keys():
        return
    vlist = []
    vkey = v[0]
    merge[vkey] = []
    for v in mresult[k]:
        for vv in v:
            for vvv in pcfset[vv]:
                for vvvv in vvv:
                    vlist.append(vvvv)
    merge[vkey].append(vlist)

for k in lt_thread:
    recursive_merge(k)
#for k in merge.keys():
#    print k, merge[k]

#for k in sorted(pcfset.keys()):
#    finalk = k
#    for m in pcfset.keys():
#        for n in pcfset[m]:
#            if k in n:
#                finalk = sorted(n)[0]
#    if finalk not in merge.keys():
#        merge[finalk] = []
#    for v in pcfset[k]:
#        if finalk not in nodes.keys():
#            merge[finalk].append(v)
#        else:
#            merge[finalk].extend(v)

tmpresult = {}

for m in merge.keys():
    for n in merge[m]:
        tmplist = []
        tmplist.append((0,starttime))
        for k in n:
            tmplist.append((nodes[k].stime,nodes[k].etime))
        tmplist.append((endtime,0))
        tmpvalue = 0
        for i in range(0,len(tmplist)-1):
            tmpvalue += tmplist[i+1][0] - tmplist[i][1]
        if (n[0],m) not in tmpresult.keys():
            tmpresult[(n[0],m)] = 0
        tmpresult[(n[0],m)] += tmpvalue/cpuFreq

mlist = {}

for k in merge.keys():
    for v in merge[k]:
        mlist[v[0]] =v 
    
# Add network backtrace
for k in send.keys():
    kpid = k
    for m in mlist.keys():
        if kpid in mlist[m]:
            kpid = m
            break
    if (-4,kpid) not in tmpresult.keys():
        tmpresult[(-4,kpid)] = 0
    tmpresult[(-4,kpid)] += send[k] * 1.0 / sendTotal * netrest 

# Add disk backtrace
dthread={}
diskTotal = 0.0
f2=open('waitfor', 'r')
for l in f2:
    l=l.split()
    if int(l[1]) == -5:
        dthread[int(l[0])] = float(l[2])
        diskTotal+=float(l[2])

for k in dthread.keys():
    kpid = k
    for m in mlist.keys():
        if kpid in mlist[m]:
            kpid = m
            break
    if (-5,kpid) not in tmpresult.keys():
        tmpresult[(-5,kpid)] = 0
    tmpresult[(-5,kpid)] += dthread[k] * 1.0 / diskTotal * diskrest 

f2.seek(0,0)
for l in f2:
    l=l.strip().split(' ')
    t1 = int(l[0])
    t2 = int(l[1])
    val = float(l[2])

    for m in mlist.keys():
        if t1 in mlist[m]:
            t1 = m
        if t2 in mlist[m]:
            t2 = m
    if (t1,t2) not in tmpresult.keys():
        tmpresult[(t1,t2)] = 0.0
    tmpresult[(t1,t2)] += val

if len(sys.argv) > 1 and sys.argv[1] == "1":
    for k in merge.keys():
        if len(merge[k]) == 1:
            continue
        else:
            for v in merge[k]:
                print v[0],
        print ""
else:
    for k in tmpresult.keys():
        print k[0], k[1], tmpresult[k]

f2.close()
f.close()
