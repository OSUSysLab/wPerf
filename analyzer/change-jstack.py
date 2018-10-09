import sys

f=open(sys.argv[1],'r')

for l in f:
    if 'nid=0x' in l:
        tmp = int(l.split('nid=')[1].split()[0], 16)
        l=l.split()
        for i in l:
            if 'nid=0x' in i:
                print 'nid='+str(tmp),
            else:
                print i,
    else:
        print l,
