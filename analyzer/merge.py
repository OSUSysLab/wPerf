import sys
import networkx as nx
import operator

res = {} 
wakeup = {}
pid = 0
pid1 = 0
lsim=float(sys.argv[2])
f=open(sys.argv[1],'r')
stack=''
for l in f:
	if 'sched:sched_switch:' in l or 'probe:try_to_wake_up:' in l:
		if pid != 0 and pid1 == 0:
			if pid not in res.keys():
				res[pid] = {}
			if stack not in res[pid].keys():
				res[pid][stack] = 0
			res[pid][stack] += 1
		elif pid !=0 and pid1 != 0:
			if (pid,pid1) not in wakeup.keys():
				wakeup[(pid,pid1)] = {}
			if stack not in wakeup[(pid,pid1)].keys():
				wakeup[(pid, pid1)][stack] = 0
			wakeup[(pid, pid1)][stack] += 1
		pid = int(l.split()[1])
		if 'probe:try_to_wake_up:' in l and 'pid=' in l:
			pid1 = int(l.split('pid=')[1].split(' ')[0])
		else:
			pid1 = 0
		stack = ''
	elif l != '\n' and 'unknown' not in l:
		stack += l

merge={}
for p1 in res.keys():
	for p2 in res.keys():
		if p1 == p2:
			continue
		else:
			keys1 = set(res[p1].keys())
			keys2 = set(res[p2].keys())
			keys = keys1 | keys2
			if len(keys) == 0:
				continue
			inter = 0
			union = 0
			for k in keys:
				a = 0
				b = 0
				if k not in res[p1].keys():
					a = 0
				else:
					a = res[p1][k]
					#a = 1
				if k not in res[p2].keys():
					b = 0
				else:
					b = res[p2][k]
					#b = 1 
				inter += 2 * min(a, b)
				union += a + b
			merge[(p1,p2)] = inter * 1.0 / union

edge=[]

for k in merge.keys():
	if merge[k] > lsim:
		edge.append(k)

G = nx.Graph()
G.add_edges_from(edge)
result=sorted(nx.connected_components(G), key=len, reverse=True)
for i in result:
	for e in i:
	    print e,
	print ''
