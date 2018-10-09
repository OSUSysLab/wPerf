import java.io.*;
import java.util.*;
import java.nio.*;
import java.nio.file.*;
import java.nio.charset.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentSkipListMap;

class Analyzer {

	static Map<Integer, GeneralRes> overAllRes = new ConcurrentHashMap<Integer, GeneralRes>();
	static Map<Integer, ConcurrentSkipListMap<Long, Segment>> segmentMap = new ConcurrentHashMap<Integer, ConcurrentSkipListMap<Long, Segment>>();
	static Map<Short, ConcurrentSkipListMap<Long, SoftEvent>> softMap = new ConcurrentHashMap<Short, ConcurrentSkipListMap<Long, SoftEvent>>();
	static Map<String, Long> resultMap = new ConcurrentHashMap<String, Long>();
	static ArrayList<Integer> tidList = new ArrayList<Integer>();
	static Map<Integer, prevState> prevList = new HashMap<Integer, prevState>();
	static ArrayList<Map<String, Long>> totalResult = new ArrayList<Map<String, Long>>();
	static Map<Integer, TreeSet<Long>> udsList = new ConcurrentHashMap<Integer, TreeSet<Long>>();
	static Map<Integer, ConcurrentSkipListMap<Long, DPEvent>> dpList = new ConcurrentHashMap<Integer, ConcurrentSkipListMap<Long, DPEvent>>();
	static Map<Integer, ConcurrentSkipListMap<Long, WaitRange>> udswList = new ConcurrentHashMap<Integer, ConcurrentSkipListMap<Long, WaitRange>>();
	static Map<Integer, ConcurrentSkipListMap<Long, Segment>> udsCheck = new ConcurrentHashMap<Integer, ConcurrentSkipListMap<Long, Segment>>();
	static Map<Integer, ConcurrentSkipListMap<Long, Segment>> udsQuick = new ConcurrentHashMap<Integer, ConcurrentSkipListMap<Long, Segment>>();
	static Map<Integer, Long> createTimeList = new HashMap<Integer, Long>();
	static Map<Integer, Long> killTimeList = new HashMap<Integer, Long>();
	static ArrayList<Event> elist = new ArrayList<Event>();
	static ArrayList<SPINResult> spinlist = new ArrayList<SPINResult>();


	static int isUDS = 0;

	// Remove duplicated function stacks
	static Map<String, String> finalPerf = new HashMap<String, String>();
	static Map<Integer, TreeSet<Long>> compPerf  = new HashMap<Integer, TreeSet<Long>>();

	static long starttime = 0;
	static long endtime = 0;

	//static double freq = 2.4e9; 
	static double freq = 0.0; 

	//Added for test
	//static long waitTime=0;
	//static long totalWaitTime = 0;

	public void Fangtest() {
		ConcurrentSkipListMap<Long, Segment> sm1 = new ConcurrentSkipListMap<Long, Segment>();
		sm1.put((long)1, new Segment(111, 2, 1, 10, 222));
		segmentMap.put(111, sm1);
		ConcurrentSkipListMap<Long, Segment> sm2 = new ConcurrentSkipListMap<Long, Segment>();
		sm2.put((long)1, new Segment(222, 2, 1, 10, -4));
		segmentMap.put(222, sm2);

		Map<String, Long> ret1 = getBreakdown(111);
		Map<String, Long> ret2 = getBreakdown(222);
		totalResult.add(ret1);
		totalResult.add(ret2);
	}

	public static void main(String args[]) throws IOException {
		Analyzer t = new Analyzer();

		if (args[8].equals("fake")) {
			isUDS = 1;
		}
		else {
			isUDS = 2;
		}

		int numT = Integer.parseInt(args[9]);

		long stime = 0;
		long etime = 0;

		stime = System.currentTimeMillis();
		t.readCPUFreq(args[5]);
		etime = System.currentTimeMillis();
		System.out.printf("[Info] readCPUFreqency spends %f second\n" , (etime - stime)*1.0/1e3);

		stime = System.currentTimeMillis();
		t.readThread(args[3]);
		etime = System.currentTimeMillis();
		System.out.printf("[Info] readThread spends %f second\n" , (etime - stime)*1.0/1e3);

		stime = System.currentTimeMillis();
		t.readFile(args[0], elist);
		etime = System.currentTimeMillis();
		System.out.printf("[Info] readFile spends %f second\n" , (etime - stime)*1.0/1e3);

		stime = System.currentTimeMillis();
		//t.readPerf(args[4]);
		etime = System.currentTimeMillis();
		System.out.printf("[Info] readPerf spends %f second\n" , (etime - stime)*1.0/1e3);

		stime = System.currentTimeMillis();
		t.readSoft(args[1]);
		etime = System.currentTimeMillis();
		System.out.printf("[Info] readSoft spends %f second\n" , (etime - stime)*1.0/1e3);
		//t.readFutex(args[2]);

		stime = System.currentTimeMillis();
		if (isUDS == 1) {
			t.readFake(args[2]);
		}
		else if (isUDS == 2) {
			t.readFakeSpin(args[2]);
		}
		//System.exit(-1);
		etime = System.currentTimeMillis();
		System.out.printf("[Info] readFake spends %f second\n" , (etime - stime)*1.0/1e3);
		//System.exit(0);

//		System.out.println(System.currentTimeMillis() + " Finish the load log file.");
		//stime = System.currentTimeMillis();
		//t.runThread(elist, 0);
		//etime = System.currentTimeMillis();
		//System.out.printf("[Info] Basic segment spends %f second\n" , (etime - stime)*1.0/1e3);

		ArrayList<Integer> dpTidList  = null;
		ArrayList<Integer> segTidList  = null;

		segTidList = new ArrayList<Integer>();
		for (int id : prevList.keySet()) {
			segTidList.add(id);
		}

		Index	index = t.createIndex(dpTidList, segmentMap);
		ArrayList<Worker> workerList = new ArrayList<Worker>();
		t.startWorker(workerList, index, numT);

		// /* Start for busy-wait
		if (isUDS == 1) {
			// /* For fake wake up
			dpTidList = new ArrayList<Integer>();
			for (int id :dpList.keySet()) {
				ConcurrentSkipListMap<Long, DPEvent> tmpList = dpList.get(id);
				//if (id == 31234 || id == 31235) {
				//	for (DPEvent m : tmpList.values()) {
				//		m.printOut(id);
				//	}
				//}
				if (dpList.get(id).size() > 0) {
					dpTidList.add(id);
					udsCheck.put(id, new ConcurrentSkipListMap<Long, Segment>()); 
					udsQuick.put(id, new ConcurrentSkipListMap<Long, Segment>()); 
				}
			}

			//segTidList = new ArrayList<Integer>();
			//for (int id : prevList.keySet()) {
			//	segTidList.add(id);
			//}

			//index = t.createIndex(dpTidList, segmentMap);

			index.reset();
			index.tidList = segTidList;

			stime = System.currentTimeMillis();
			for ( Worker w : workerList) {
				synchronized(w.cond) {
					w.op = 5;
					w.cond.notifyAll();
				}
			}

			synchronized(index) {
				while(index.finishNum < numT) {
					try {
						index.wait();
					}
					catch (InterruptedException e) {
					}
				}
			}

			etime = System.currentTimeMillis();
			System.out.printf("[Info] Basic segment spends %f second\n" , (etime - stime)*1.0/1e3);


			index.reset();
			index.tidList = dpTidList;

			stime = System.currentTimeMillis();
			for ( Worker w : workerList) {
				synchronized(w.cond) {
					w.op = 1;
					w.cond.notifyAll();
				}
			}

			synchronized(index) {
				while(index.finishNum < numT) {
					try {
						index.wait();
					}
					catch (InterruptedException e) {
					}
				}
			}

			etime = System.currentTimeMillis();
			System.out.printf("[Info] FakePrep spends %f second\n" , (etime - stime)*1.0/1e3);

			stime = System.currentTimeMillis();
			index.reset();

			for ( Worker w : workerList) {
				synchronized(w.cond) {
					w.op = 2;
					w.cond.notifyAll();
				}
			}

			synchronized(index) {
				while(index.finishNum < numT) {
					try {
						index.wait();
					}
					catch (InterruptedException e) {
					}
				}
			}

			etime = System.currentTimeMillis();
			System.out.printf("[Info] FakeClean spends %f second\n" , (etime - stime)*1.0/1e3);
			// END fake Wakeup */
		}
		else if (isUDS == 2) {
			segTidList = new ArrayList<Integer>();
			for (int id : prevList.keySet()) {
				segTidList.add(id);
			}

			dpTidList = new ArrayList<Integer>();
			for (int id :udsCheck.keySet()) {
				dpTidList.add(id);
				//for (Segment s : udsCheck.get(id).values()) {
				//	s.printOut();
				//}
			}

			index = t.createIndex(dpTidList, segmentMap);

			workerList = new ArrayList<Worker>();
			t.startWorker(workerList, index, numT);

			index.reset();
			index.tidList = segTidList;

			stime = System.currentTimeMillis();
			for ( Worker w : workerList) {
				synchronized(w.cond) {
					w.op = 5;
					w.cond.notifyAll();
				}
			}

			synchronized(index) {
				while(index.finishNum < numT) {
					try {
						index.wait();
					}
					catch (InterruptedException e) {
					}
				}
			}

			etime = System.currentTimeMillis();
			System.out.printf("[Info] Basic segment spends %f second\n" , (etime - stime)*1.0/1e3);


			stime = System.currentTimeMillis();

			index.reset();
			index.tidList = dpTidList;

			for ( Worker w : workerList) {
				synchronized(w.cond) {
					w.op = 4;
					w.cond.notifyAll();
				}
			}

			synchronized(index) {
				while(index.finishNum < numT) {
					try {
						index.wait();
					}
					catch (InterruptedException e) {
					}
				}
			}

			etime = System.currentTimeMillis();
			System.out.printf("[Info] FakeClean spends %f second\n" , (etime - stime)*1.0/1e3);
			// End for busy-wait  */
		}

		stime = System.currentTimeMillis();

		index.reset();
		index.tidList = segTidList;

		for ( Worker w : workerList) {
			synchronized(w.cond) {
				w.op = 3;
				w.cond.notifyAll();
			}
		}

		synchronized(index) {
			while(index.finishNum < numT) {
				try {
					index.wait();
				}
				catch (InterruptedException e) {
				}
			}
		}

		etime = System.currentTimeMillis();
		System.out.printf("[Info] Cascading spends %f second\n" , (etime - stime)*1.0/1e3);

		stime = System.currentTimeMillis();
		for (Worker w : workerList) {
				totalResult.add(w.hm);
		}

		for (Map<String, Long> tmp : totalResult) {
			if (tmp == null || tmp.size()==0) {
				//System.out.println("Empty result!\n");
				continue;
			}
			for (String str : tmp.keySet()) {
				if (resultMap.containsKey(str)) {
					resultMap.put(str, resultMap.get(str) + tmp.get(str));
				}
				else {
					resultMap.put(str, tmp.get(str));
				}
			}
		}

		System.out.println(System.currentTimeMillis() + " Finish the final edge weight.");
		PrintWriter fwait = new PrintWriter(args[6], "ASCII");

		for (String s : resultMap.keySet()) {
			if (s.contains("-99")) continue;
			//System.out.printf("%s%f\n", s,resultMap.get(s)/100000000.0);
			fwait.printf("%s%f\n", s,resultMap.get(s)/freq);
		}

		PrintWriter fbreak = new PrintWriter(args[7], "ASCII");
		fbreak.printf("ThreadID Running Runnable Wait HardIRQ SoftIRQ Network Disk Other Unknown Period\n");
		for (Integer tid : prevList.keySet()) {
			if (prevList.get(tid).state==-1) {
				continue;
			}
			overAllRes.get(tid).printOut(tid,fbreak);
		}

		fbreak.close();
		fwait.close();

		etime = System.currentTimeMillis();
		System.out.printf("[Info] Collection spends %f second\n" , (etime - stime)*1.0/1e3);


		long infiniteTotal = 0L;
		long processTotal = 0L;
		for (Worker w : workerList) {
			infiniteTotal += w.infiniteTime;
			processTotal += w.processTime;
		}
		System.out.printf("[Info] Infinite Loop ignores cycle = %f, total processed cycles = %f, percent = %f\n" , infiniteTotal/freq, processTotal/freq, infiniteTotal*100.0/processTotal);
		System.exit(0);
	}

	public void runThread(ArrayList<Event> elist, int op) {
		ArrayList<doSegment> tmpList = new ArrayList<doSegment>();
		if (op == 0) {
			//for (Integer tid : tidList) {
			for (Integer tid : prevList.keySet()) {
				doSegment dos = new doSegment(tid, elist, op);
				tmpList.add(dos);
			}
		}
		else {
			for (Integer tid : prevList.keySet()) {
				if (prevList.get(tid).state==-1) {
					continue;
				}
				doSegment dos = new doSegment(tid, elist, op);
				tmpList.add(dos);
			}
		}

		//if (op == 0) {
			for (doSegment dos : tmpList) {
				dos.start();
			}

			for (doSegment dos : tmpList) {
				dos.join();
			}
		//}
		//else {
		//	int j = 0;
		//	while (j < tmpList.size()) {
		//		int i = 0;
		//		for (i=0;i<10;i++) {
		//			if ((j+i)<tmpList.size()) {
		//				tmpList.get(j+i).start();
		//			}
		//			else {
		//				continue;
		//			}
		//		}
		//		for (i=0;i<10;i++) {
		//			if ((j+i)<tmpList.size()) {
		//				tmpList.get(j+i).join();
		//			}
		//			else {
		//				continue;
		//			}
		//		}
		//		j+=10;
		//	}
		//}
	}

	// New added by Fang in summer
	class prevState {
		int state;
		long time;

		public prevState(int state, long time) {
			this.state = state;
			this.time = time;
		}
	}

	class doSegment implements Runnable{
		private Thread t;
		int tid;
		int op;
		ArrayList<Event> elist;
		public doSegment(int tid, ArrayList<Event> elist, int op) {
			this.tid = tid;
			this.elist = elist;
			this.op = op;
		}

		public void run() {
			if (op == 0) {
				directBreakdown(elist, tid);
				System.out.println("[directBreakdown] Thread " + tid + " has been finished");
			}
			else if (op == 1) {
				//removeFakeWakeup(tid);
				System.out.println("[removeFakeWakeup] Thread " + tid + " has been finished");
			}
			else {
				Map<String, Long> ret = getBreakdown(tid);
				totalResult.add(ret);
				System.out.println("[getBreakdown] Thread " + tid + " has been finished");
			}
		}

		public void join() {
			try {
				t.join();
			}
			catch (InterruptedException e) {
			}
		}

		public void start () {
			t = new Thread (this);
			t.start();
		}
	}

	public class UDSResult implements Comparable<UDSResult> {
		long ts;
		int tid;
		long lock;
		short type;

		public UDSResult(ByteBuffer buf) throws IOException {
			this.ts = buf.getLong();
			this.tid = buf.getInt();
			this.lock = buf.getLong();
			this.type = buf.getShort();
		}

		@Override
		public int compareTo(UDSResult e)
		{
			if (this.ts > e.ts) return 1;
			else if (this.ts < e.ts) return -1;
			else return 0;
		}

		public void printOut() {
			System.out.printf("%d %d %d %d\n", this.ts, this.tid, this.lock, this.type);
		}
	}

//	public void removeFakeWakeup(int tid) {
//		TreeSet<Long> tmpList = udsList.get(tid);
//		ConcurrentSkipListMap<Long, Segment> tm = segmentMap.get(tid);
//		long lastTime = tm.lastKey();
//		long firstTime = tm.firstKey();
//		for (UDSResult uds: tmpList) {
//			if (lastTime <= uds.ts && firstTime >= uds.ts) continue;
//
//			Segment sTmp = null;
//			Segment tmp = null;
//
//			sTmp = (Segment) tm.lowerEntry(uds.ts).getValue();
//			uds.printOut();
//			sTmp.printOut();
//			while(true) {
//				if (sTmp.state == 2) {
//					break;
//				}
//				if (tm.lowerEntry(sTmp.startTime) == null) {
//					sTmp = (Segment) tm.firstEntry().getValue();
//					break;
//				}
//				sTmp = (Segment) tm.lowerEntry(sTmp.startTime).getValue();
//			}
//
//			long sTime = sTmp.startTime;
//			long startTime = sTmp.startTime;
//			//long eTime = 0;
//			Segment newSeg;
//
//			while(true) {
//				tm.remove(sTime);
//				if (tm.higherEntry(sTime) == null) {
//					newSeg = new Segment(tid, 2, sTime, lastTime, -99);
//					break;
//				}
//				tmp = (Segment) tm.higherEntry(sTime).getValue();
//				if (tmp.state == 2) {
//					newSeg = new Segment(tid, 2, startTime, tmp.endTime, tmp.waitFor);
//					tm.remove(tmp.startTime);
//					break;
//				}
//				sTime = tmp.startTime;
//			}
//			tm.put(newSeg.startTime, newSeg);
//		}
//	}

	public void directBreakdown(ArrayList<Event> elist, int tid) {
		//int WAITSTART = 1;
		//int IOSTART = 2;
		//double traceTime = elist.get(0).time;
		//double time = 0;

		prevState ps = prevList.get(tid);
		ConcurrentSkipListMap<Long, Segment> tm = segmentMap.get(tid);
		GeneralRes gr = overAllRes.get(tid);

		for (Event e : elist) {
			// switch_to
			long prevtime = ps.time;
			if (e.type== 0) {
				if (e.pid1== tid) {
					if (e.pid1state == 0) {
						//Update previous state and time
						ps.state = 1;
						ps.time = e.time;
					}
					else {
						ps.state = 2;
						ps.time = e.time;
					}

					try {
						SoftEvent se = softMap.get(e.core).higherEntry(prevtime).getValue();

						//tm.put(prevtime, new Segment(tid, 0, prevtime, e.time, 0));

						// For global info
						gr.running += e.time - prevtime;

						if (( se.stime < e.time) && (se.etime < e.time)) {
							tm.put(prevtime, new Segment(tid, 0, prevtime, se.stime, 0));
							tm.put(se.stime, new Segment(tid, 1, se.stime, se.etime, 0));
							//tm.put(se.stime, new Segment(tid, 2, se.stime, se.etime, -16));
							tm.put(se.etime, new Segment(tid, 0, se.etime, e.time, 0));

							// For global info
							gr.running += se.stime-prevtime;
							gr.softirq += se.etime - se.stime;
							gr.running += e.time - se.etime;
						}
						else {
							tm.put(prevtime, new Segment(tid, 0, prevtime, e.time, 0));

							// For global info
							gr.running += e.time - prevtime;
						}
					}
					catch (Exception error) {
						tm.put(prevtime, new Segment(tid, 0, prevtime, e.time, 0));
						// For global info
						gr.running += e.time - prevtime;
					}
				}
				else if (e.pid2== tid) {
					//In __switch_to, the switch-in thread state is absolutely 0.
					tm.put(prevtime, new Segment(tid, 1, prevtime, e.time, 0));
					ps.state = 0;
					ps.time = e.time;

					// For global info
					gr.runnable += e.time - prevtime; 
				}
			}
			// try_to_wake_up
			else {
				if (e.pid2== tid) {
					if (ps.state != 2) continue;

					//// Added for test
					//synchronized((Object) totalWaitTime) {
					//totalWaitTime += e.time - prevtime;
					//}

					if (e.irq== 0) {
						tm.put(prevtime, new Segment(tid, 2, prevtime, e.time, e.pid1));
						ps.state = 1;
						ps.time = e.time;

						// For global info
						gr.wait += e.time - prevtime; 
					}
					else {
						tm.put(prevtime, new Segment(tid, 2, prevtime, e.time, e.irq));
						ps.state = 1;
						ps.time = e.time;

						// For global info
						if (e.irq == -5) gr.disk += e.time - prevtime; 
						else if (e.irq == -4) gr.network += e.time - prevtime;
						else if (e.irq == -15) gr.hardirq += e.time - prevtime;
						else if (e.irq == -16) gr.softirq += e.time - prevtime;
						else gr.other += e.time - prevtime;
					}
				}
			}
		}

		// Final round
		ps = prevList.get(tid);
		if (ps.state == 0) {
			tm.put(ps.time, new Segment(tid, 0, ps.time, endtime, 0));

			// For global info
			gr.running += endtime - ps.time;
		}
		else if (ps.state == 1) {
			tm.put(ps.time, new Segment(tid, 1, ps.time, endtime, 0));

			// For global info
			gr.runnable += endtime - ps.time;
		}
		else {
			if (killTimeList.containsKey(tid)) {
				// Solve the kill time
				long killTime = killTimeList.get(tid);
				tm.lowerEntry(killTime).getValue().endTime = killTime;
			}
			else {
				tm.put(ps.time, new Segment(tid, 2, ps.time, endtime, -99));
			}

			// For global info
			gr.unknown += endtime - ps.time;
		}
		//	time = elist.get(elist.size()-1).time;
		//	if (time > timeTable.get(tid).time) {
		//		timeState tstmp = timeTable.get(tid);
		//		ConcurrentSkipListMap<Double, Segment> tm = segmentMap.get(tid);
		//		if (tstmp.state == 0) {
		//			tm.put(tstmp.time, new Segment(tid, 0, tstmp.time, time, 0));
		//		}
		//		else if (tstmp.state == 1) {
		//			tm.put(tstmp.time, new Segment(tid, 1, tstmp.time, time, tid));
		//		}
		//		else if (tstmp.state == 2) {
		//			tm.put(tstmp.time, new Segment(tid, 2, tstmp.time, time, 0));
		//		}
		//	}
	}

	public void readCPUFreq(String fname) throws IOException {
		Path fpath = Paths.get(fname);
		List<String> lines = Files.readAllLines( fpath, StandardCharsets.US_ASCII);
		double ghz = 0.0;
		for (String line : lines) {
			ghz = Double.valueOf(line.trim());
		}
		freq = ghz * 1e9;
	}

	public void readThread(String fname) throws IOException {
		Path fpath = Paths.get(fname);
		List<String> lines = Files.readAllLines( fpath, StandardCharsets.US_ASCII);
		for (String line : lines) {
			//System.out.println(line);
			String[] tmp = line.split(" ");
			int pid = Integer.parseInt(tmp[0]);
			if (tidList.contains(pid)) {
				continue;
			}
			tidList.add(pid);	

			segmentMap.put(pid, new ConcurrentSkipListMap<Long, Segment>());
			overAllRes.put(pid, new GeneralRes());
			compPerf.put(pid, new TreeSet<Long>());
			//udsList.put(pid, new TreeSet<Long>());
			//udswList.put(pid, new ConcurrentSkipListMap<Long, WaitRange>());
			//dpList.put(pid, new ConcurrentSkipListMap<Long, DPEvent>());
		}
	}

	public class Segment {
		int pid;
		int state;	// 0 means running, 1 means runnable, 2 means wait, 3 means unknown wait
		long startTime;
		long endTime;
		int waitFor;	// Which thread the current thread waits for
		int core;
		int parent;
		long ftime;
		ArrayList<Integer> overlap;

		public Segment(int pid, int state, long startTime, long endTime, int waitFor) {
			this.pid = pid;
			this.state = state;
			this.startTime = startTime;
			this.endTime = endTime;
			this.waitFor = waitFor;
			this.ftime = 0;
			this.overlap = new ArrayList<Integer>();
		}

		public Segment(int pid, int state, long startTime, long endTime, int waitFor, int parent) {
			this.pid = pid;
			this.state = state;
			this.startTime = startTime;
			this.endTime = endTime;
			this.waitFor = waitFor;
			this.ftime = 0;
			this.parent = parent;
			this.overlap = new ArrayList<Integer>();
		}

		public void printOut() {
			System.out.printf("Tid %d State %d StartTime %d EndTime %d ftime %d waitFor %d parent %d\n", this.pid, this.state, this.startTime, this.endTime, this.ftime, this.waitFor, this.parent);
		}
	}

	public class timeState {
		int state;
		double time;
		public timeState(int state, double time) {
			this.state = state;
			this.time = time;
		}
	}

	public void addResult(HashMap<String, Long> resultMap, int pid, int wid, long start, long end) {
		String strtmp = String.valueOf(pid) + " " + String.valueOf(wid) + " ";
		//FFang Adddd
		//System.out.println(strtmp);
		if(resultMap.containsKey(strtmp)) {
			resultMap.put(strtmp, resultMap.get(strtmp)+end-start);
		}
		else {
			resultMap.put(strtmp, end-start);
		}
	}

	//Fang new
	public boolean isFakeWakeNew(int tid, Long time) {
		ConcurrentSkipListMap<Long, Segment> udsMap = udsCheck.get(tid);
		if (udsMap == null) return false;

		if (udsMap.lowerEntry(time) != null) {
			Segment tmp = udsMap.lowerEntry(time).getValue();
			if (tmp.startTime <= time && tmp.endTime >= time) {
				return true;
			}
		}
		return false;
	}
	// Stats for result
	//int total=0;
	//int ftotal=0;

	// Check if a wait segment is caused by a fake wake-up event
	public boolean isFakeWake(int tid, Segment seg) {
		TreeSet<Long> ts = udsList.get(tid);
		ConcurrentSkipListMap<Long, DPEvent> currDP = dpList.get(tid);
		ConcurrentSkipListMap<Long, Segment> tm = segmentMap.get(tid);
		Segment runSeg1 = null;

		long stime = seg.endTime;
		long etime = 0;
		long tmptime = stime;

		//total++;

		// No need to check hardware 
		if (tid < 0) return false;

		// Find out the next several running or runnable segments
		try {
			while(true) {
				// Find the first segment based on the prior segment's endTime
				runSeg1 = tm.higherEntry(tmptime).getValue();
				if (runSeg1.state != 2) {
					etime = runSeg1.endTime;
				}
				else {
					break;
				}
				tmptime = runSeg1.startTime;
			}
		}
		catch (Exception e) {
			return false;
		}

		// Prepare the ArrayList of DPEvent
		ArrayList<DPEvent> dplist = new ArrayList<DPEvent>();
		try {
			tmptime = stime;
			while(true) {
				DPEvent tmpDP = currDP.higherEntry(tmptime).getValue();
				if (tmpDP.time > etime) {
					break;
				}
				dplist.add(tmpDP);	
				tmptime = tmpDP.time;
			}
		}
		catch (Exception e) {
		}

		DPEvent preDP = null;
		try {
			preDP = currDP.lowerEntry(tmptime).getValue();
		}
		catch (Exception e) {
		}

		// Nothing in DPList:
		if (dplist.size()==0)
			return true;
		// Only pending DP1, no DP2 in this period
		if (preDP != null && preDP.type==15) {
			int find = 0;
			DPEvent dp = dplist.get(0);
			if (dp.type != 16) {
				return true;
			}
			//for (DPEvent dp : dplist) {
			//	if (dp.type == 16) {
			//		find = 1;
			//		break;
			//	}
			//}
			//if (find == 0) {
			//	//ftotal++;
			//	return true;
			//}
		}

		// DP2, DP3, and DP1 in this period
		if (dplist.size()>=3) {
			if (dplist.get(0).type == 16 && dplist.get(1).type == 0 && dplist.get(2).type == 15) {
				//ftotal++;
				return true;
			}
		}
		return false;
	}

	public Segment getRealWake(Segment seg, ConcurrentSkipListMap<Long, Segment> tm) {
		Segment res = new Segment(seg.pid, seg.state, seg.startTime, seg.endTime, seg.waitFor);
		Segment tmp = res;
		long tmpTime = res.startTime;

		do {
			if (tmp.state == 2 && !isFakeWake(tmp.pid, tmp)) {
				res.endTime = tmp.endTime;
				res.waitFor = tmp.waitFor;
				break;
			}
			tmpTime = tmp.startTime;
			try {
				tmp = tm.higherEntry(tmpTime).getValue();
			}
			catch (Exception e) {
				break;
			}
		} while(true);
		//if (res.waitFor != seg.waitFor || res.endTime != seg.endTime) {
		//	seg.printOut();
		//	res.printOut();
		//}
		return res;
	}

	public HashMap<String, Long> getBreakdown(int pid) {
		HashMap<String, Long> hm = new HashMap<String, Long>();
		ConcurrentSkipListMap<Long, Segment> tm = segmentMap.get(pid);
		Segment tmpSeg = null;
		int reportInterval = 0;
		long lastSeg = -1;
		if (tm == null) {
			return hm;
		}

		//int[] sync = {18046,18047,18048,18049,18050};
		//int[] st = {18551, 18562, 18581, 18591, 18608, 18618};
		//int[] rp = {18554, 18565, 18584, 18594, 18611, 18621};
		//int[] handle = {17993,17994,17995,17996,17997,17998,17999,18000,18001,18002};

		for (Segment seg : new ArrayList<Segment>(tm.values())) {
			//FFang for test
			//System.out.println("pid="+pid+", state="+seg.state);
			
			if (seg.state == 0 || seg.state == 1) {
			}
			else if (seg.state == 2) {
				// Avoid multiple calculations
				if (seg.startTime < lastSeg) {
					continue;
				}

				// First find out the real wake up period and update to get a new segment.
				Segment newSeg = getRealWake(seg, tm);
				lastSeg = newSeg.endTime;

				//newSeg.printOut();
				
				ArrayList<Segment> tmpList = new ArrayList<Segment>();
				tmpList.add(newSeg);
				//newSeg.printOut();
				//System.out.println("StartFindAll,pid="+pid+", startTime="+seg.startTime+", endTime="+seg.endTime+", waitFor="+seg.waitFor);
				while(!tmpList.isEmpty()) {
					Segment segTmp = tmpList.remove(0);
					// If the temp segment's time range is wrong
					if (segTmp.startTime > segTmp.endTime)
						continue;
					if (segTmp.state == 2) {
						if (segTmp.startTime < newSeg.startTime) {
							System.out.println("[Start Warning]");
							newSeg.printOut();
							segTmp.printOut();
							System.out.println("[Finish Warning]");
						}
						//segTmp.printOut();
						reportInterval++;
						if (reportInterval % 1000000 == 0) {
							reportInterval = 0;
							System.out.printf("[INFO] ");
							segTmp.printOut();
						}
						// Check if the wake-up event is a fake one
						ConcurrentSkipListMap<Long, Segment> tmWake = segmentMap.get(segTmp.waitFor);
						long wakeTime = segTmp.endTime;
						Segment sTmp = null;
						try {
							while(tmWake.lowerEntry(wakeTime)!=null) {
								sTmp = tmWake.lowerEntry(wakeTime).getValue();
								if (sTmp.state == 2) {
									break;
								}
								wakeTime = sTmp.startTime;
							}
						}
						// This means we cannot make sure if the waitFor thread is caused by a real or fake wake up.
						catch (Exception e) {
							//continue;
						}

						if (sTmp == null || !isFakeWake(sTmp.pid, sTmp))
						{
							addResult(hm, segTmp.pid, segTmp.waitFor, segTmp.startTime, segTmp.endTime);
							if (segTmp.waitFor <= 0 || segTmp.waitFor == segTmp.pid) {
								continue;
							}
							else {
								//tmpList.addAll(findAll(segTmp.waitFor, segTmp.startTime, segTmp.endTime, segTmp.parent, segTmp, this));
							}
						}
						else {
							try {
								tmpList.add(new Segment(segTmp.pid, 2, segTmp.startTime, sTmp.endTime, sTmp.waitFor, segTmp.waitFor));
							}
							catch (Exception e) {
								continue;
							}
						}
						// If not, we'll directly add this segment info into final result
					}
				}
				tmpList.clear();
				tmpList = null;
			}
		}
		return hm;
	}

	// Once I find unqueue_me, I will change the state to 0.
	public ArrayList<Segment> findAll(int pid, long start, long end, int parent, Segment segT, Worker w) {
		ArrayList<Segment> ret = new ArrayList<Segment>();
		if (start>end) {
			return ret;
		}
		//else {
		//	System.out.println("pid="+pid+",start="+start+",end="+end+",parent="+parent);
		//}
		long key = 0;
		//Find the first breakdown for thread pid
		ConcurrentSkipListMap<Long, Segment> tm = segmentMap.get(pid);
		if (tm == null) {
			//System.out.println("pid " + pid + " and no tm");
			return ret;
		}
		try {
			key = tm.floorKey(start);
		} catch (Exception e) {
			//System.out.println("[floorKey] pid = " + pid + ", start time = " + start + ", parent = " + parent);
			return ret;
		}
		Segment seg = tm.get(key);

		while(true) {
			Segment tmp = new Segment(pid, seg.state, Math.max(start, seg.startTime), Math.min(end, seg.endTime), seg.waitFor, parent); 
			//start for spinlock
			tmp.overlap = new ArrayList<Integer>(segT.overlap);
			if (tmp.overlap.contains(pid)) {
				w.infiniteTime += 1.0 * (tmp.endTime - tmp.startTime) * tmp.overlap.size();
			}
			else {
				tmp.overlap.add(pid);
				ret.add(tmp);
			}
			//end for spinlock

			if (seg.endTime >= end)
				break;
			try {
				seg = tm.get(tm.higherKey(seg.startTime));
			} catch (Exception e) {
				System.out.println("[higherKey] pid = " + pid + ", start time = " + start + ", parent = " + parent);
				return ret;
			}
			if (seg.startTime >= end) break;
		}
		return ret;
	}

	public void readSoft(String fname) throws IOException {
		int nRead = 0;
		int numEvent = 10000000;
		int sum = 0;

		byte[] buffer = new byte[19*10000000];
		DataInputStream is = new DataInputStream(new FileInputStream(fname));

		long statSTime = System.currentTimeMillis();
		while( nRead != -1 )
		{
			int i = 0;
			nRead = is.read(buffer);
			ByteBuffer buf = ByteBuffer.wrap(buffer);
			buf.order(ByteOrder.LITTLE_ENDIAN);
			for (i = 0; i < nRead/19; i++) {
				SoftEvent e = new SoftEvent(buf);
				//e.printOut();
				if (!softMap.containsKey(e.core))
					softMap.put(e.core, new ConcurrentSkipListMap<Long, SoftEvent>());
				softMap.get(e.core).put(e.stime,e);
			}
			sum += i;
			if (i != 0) {
				System.out.println(System.currentTimeMillis() + " Finish loading " + sum + " softirq events.");
			}
			//FFang Adddd
			//if (sum > nRead/26*10) break;
		}

		long statETime = System.currentTimeMillis();
		System.out.printf("[INFO] Finish loading softirq events, spending %f second\n" , (statETime - statSTime)*1.0/1e3);
	}

	public class FEvent {
		int pid;
		long time;
		public FEvent(ByteBuffer buf) throws IOException {
			this.pid = buf.getInt();
			this.time = buf.getLong();
		}
		public void printOut() {
			System.out.printf("%d %d\n", this.pid, this.time);
		}
	}

	public class SPINResult implements Comparable<SPINResult> {
		long ts;
		int pid;
		long lock;
		short type;

		public SPINResult(ByteBuffer buf) throws IOException {
			this.ts = buf.getLong();
			this.pid = buf.getInt();
			this.lock = buf.getLong();
			this.type = buf.getShort();
		}

		@Override
		public int compareTo(SPINResult e)
		{
			if (this.ts > e.ts) return 1;
			else if (this.ts < e.ts) return -1;
			else return 0;
		}

		public void printOut() {
			System.out.printf("%d %d %d %d\n", this.ts, this.pid, this.lock, this.type);
		}
	}

	public void readFakeSpin(String fname) throws IOException {
		int nRead = 0;
		int numEvent = 10000000;
		int sum = 0;

		int s0 = 0;
		int s1 = 0;
		int s2 = 0;
		int s3 = 0;
		int s4 = 0;
		int s5 = 0;
		int slock = 0;
		int sother = 0;

		byte[] buffer = new byte[22*10000000];
		DataInputStream is = new DataInputStream(new FileInputStream(fname));

		HashMap<Long, Integer> hm = new HashMap<Long, Integer>();
		HashMap<Integer, Long> thm = new HashMap<Integer, Long>();
		HashMap<Integer, Integer> thm1 = new HashMap<Integer, Integer>();
		HashMap<Integer, Integer> ths = new HashMap<Integer, Integer>();

		long statSTime = System.currentTimeMillis();
		while( nRead != -1 )
		{
			int i = 0;
			nRead = is.read(buffer);
			ByteBuffer buf = ByteBuffer.wrap(buffer);
			buf.order(ByteOrder.LITTLE_ENDIAN);
			for (i = 0; i < nRead/22; i++) {
				SPINResult spinRes = new SPINResult(buf);
				//spinRes.printOut();
				spinlist.add(spinRes);
			}
			sum += i;
			if (i != 0) {
				System.out.println(System.currentTimeMillis() + " Finish loading " + sum + " events.");
			}
		}

		long statETime = System.currentTimeMillis();
		statSTime = statETime;
		System.out.printf("[INFO] Finish loading fake wakeup events, spending %f second\n" , (statETime - statSTime)*1.0/1e3);

		Collections.sort(spinlist);

		for (SPINResult spinRes : spinlist) {
			if (spinRes.ts < starttime || spinRes.ts > endtime)
				continue;

			//if (spinRes.type != -1)
			//spinRes.printOut();

			// type == -3 or -1 means the thread has got the latch
			if (spinRes.type == -1 || spinRes.type == -2) {
				if (thm.get(spinRes.pid) == null || thm.get(spinRes.pid) == 0 || thm1.get(spinRes.pid) == null) {
				}
				else {
					ConcurrentSkipListMap<Long, Segment> csm = null;
					if (!udsCheck.containsKey(spinRes.pid)) {
						udsCheck.put(spinRes.pid, new ConcurrentSkipListMap<Long, Segment>());
					}
					csm = udsCheck.get(spinRes.pid);
					if (hm.get(spinRes.lock) != null) {
							//csm.put(thm.get(spinRes.pid), new Segment(spinRes.pid, 2, thm.get(spinRes.pid), spinRes.ts, hm.get(spinRes.lock), 0));
							csm.put(thm.get(spinRes.pid), new Segment(spinRes.pid, 2, thm.get(spinRes.pid), spinRes.ts, thm1.get(spinRes.pid), -99));
					}
				}

				if (spinRes.type == -1) {
					hm.put(spinRes.lock, spinRes.pid);
				}
				thm.put(spinRes.pid, 0L);
				//ths.put(spinRes.pid, 0);
			}
			// type > 0 means the thread starts to wait on a latch
			else if (spinRes.type >= 0) {
				// Avoid duplicated updating on the start time of waiting
				//if (ths.get(spinRes.pid) == null || ths.get(spinRes.pid) == 0) {
				thm.put(spinRes.pid, spinRes.ts);
				thm1.put(spinRes.pid, hm.get(spinRes.lock));
				//}
				//ths.put(spinRes.pid, 1);
			}
		}

		spinlist.clear();
		statETime = System.currentTimeMillis();
		System.out.printf("[INFO] Finish analyzing fake wakeup events, spending %f second\n" , (statETime - statSTime)*1.0/1e3);
	}


	// New for fake wakeup running time
	public void readFake(String fname) throws IOException {
		int nRead = 0;
		int numEvent = 10000000;
		int sum = 0;

		byte[] buffer = new byte[22*10000000];
		DataInputStream is = new DataInputStream(new FileInputStream(fname));

		HashMap<Integer, Long> tmpStart = new HashMap<Integer, Long>();
		long wstime = 0;
		long wetime = 0;
	
		long statSTime = System.currentTimeMillis();
		while( nRead != -1 )
		{
			int i = 0;
			nRead = is.read(buffer);
			ByteBuffer buf = ByteBuffer.wrap(buffer);
			buf.order(ByteOrder.LITTLE_ENDIAN);
			for (i = 0; i < nRead/22; i++) {
				UDSResult udsr = new UDSResult(buf);
				if (udsList.get(udsr.tid)==null) {
					tidList.add(udsr.tid);	
					prevList.put(udsr.tid, new prevState(-1, starttime));
					segmentMap.put(udsr.tid, new ConcurrentSkipListMap<Long, Segment>());
					overAllRes.put(udsr.tid, new GeneralRes());
					udsList.put(udsr.tid, new TreeSet<Long>());
					udswList.put(udsr.tid, new ConcurrentSkipListMap<Long, WaitRange>());
					dpList.put(udsr.tid, new ConcurrentSkipListMap<Long, DPEvent>());
				}

				// dpList generation
				if (udsr.type == 0 || udsr.type == 15 || udsr.type == 16) {
					dpList.get(udsr.tid).put(udsr.ts, new DPEvent(udsr.ts, udsr.type));
				}

				if (udsr.type == 0) {
					udsList.get(udsr.tid).add(udsr.ts);
					// Fang Test
					// udsr.printOut();
				}
				else if (udsr.type == 15) {
					tmpStart.put(udsr.tid, udsr.ts);
				}
				else if (udsr.type == 16) {
					if (tmpStart.get(udsr.tid)==null) {
						continue;
					}
					else {
						wstime = tmpStart.get(udsr.tid);
						wetime = udsr.ts;
						udswList.get(udsr.tid).put(wstime, new WaitRange(wstime, wetime));
						//System.out.printf("pid = %d, wstime = %d, wetime = %d\n", udsr.tid, wstime, wetime);
					}
				}
				
				//udsr.printOut();
				//if (udsr.type == 0) {
				//}
				//e.printOut();
				//elist.add(e);
			}
			//System.exit(-1);
			sum += i;
			if (i != 0) {
				System.out.println(System.currentTimeMillis() + " Finish loading " + sum + " events.");
			}
			//FFang Adddd
			//break;
			//if (sum > nRead/26*10) break;
		}

		long statETime = System.currentTimeMillis();
		System.out.printf("[INFO] Finish loading fake wakeup events, spending %f second\n" , (statETime - statSTime)*1.0/1e3);

		// Sort
		//Collections.sort(elist);
		//starttime = elist.get(0).time;
		//endtime = elist.get(elist.size()-1).time;
		//System.out.println("Start time = " + starttime + ", End time = " + endtime);
	}

	// New for futex running time
	public void readFutex(String fname) throws IOException {
		int nRead = 0;
		int numEvent = 10000000;
		int sum = 0;

		byte[] buffer = new byte[12*10000000];
		DataInputStream is = new DataInputStream(new FileInputStream(fname));

		while( nRead != -1 )
		{
			int i = 0;
			nRead = is.read(buffer);
			ByteBuffer buf = ByteBuffer.wrap(buffer);
			buf.order(ByteOrder.LITTLE_ENDIAN);
			for (i = 0; i < nRead/12; i++) {
				FEvent e = new FEvent(buf);
				e.printOut();
			}
			sum += i;
			if (i != 0) {
				System.out.println(System.currentTimeMillis() + " Finish loading " + sum + " events.");
			}
		}
	}

	public void readPerf(String fname) throws IOException {
		BufferedReader br = new BufferedReader(new FileReader(new File(fname)));

		int pid = 0;
		long time = 0L;
		long rtime = 0L;
		String key = null;
		String value = null;
		String line = null;
		while ((line = br.readLine()) != null) {
			if (line.contains("pid=")) {
				if (pid != 0) {
					//System.out.println(pid + "," + rtime);
					try {
						rtime = compPerf.get(pid).higher(time);
					}
					catch (Exception e) {
						rtime = time;
					}
					key = Integer.toString(pid) + "-" + Long.toString(rtime);
					finalPerf.put(key, value);
				}
				pid = Integer.valueOf(line.replaceAll(" +"," ").split(" ")[1]);
				time = (long)(Double.valueOf(line.replaceAll(" +"," ").split(" ")[3].replace(":","")) * 1e9);
				value = line;
			}
			else {
				value += "\n" + line;
			}
		}

		PrintWriter fperf = new PrintWriter("presult.final", "ASCII");
		for (String k : finalPerf.keySet()) {
			//System.out.println(k);
			fperf.printf(finalPerf.get(k)+"\n");		
		}
		fperf.close();		
		//System.exit(0);
	}

	public void readFile(String fname, ArrayList<Event> elist) throws IOException {
		int nRead = 0;
		int numEvent = 10000000;
		int sum = 0;
		long statSTime = System.currentTimeMillis();

		byte[] buffer = new byte[34*10000000];
		DataInputStream is = new DataInputStream(new FileInputStream(fname));

		while( nRead != -1 )
		{
			int i = 0;
			nRead = is.read(buffer);
			ByteBuffer buf = ByteBuffer.wrap(buffer);
			buf.order(ByteOrder.LITTLE_ENDIAN);
			for (i = 0; i < nRead/34; i++) {
				Event e = new Event(buf);
				//e.printOut();
				//If the event is not 0 and 1, record in another list
				if (e.type == 2) {
					createTimeList.put(e.pid2, e.time);
				}
				else if (e.type == 3) {
					killTimeList.put(e.pid1, e.time);
				}
				elist.add(e);
				if (prevList.containsKey(e.pid1)) {
					compPerf.get(e.pid1).add(e.perftime);
				}
				//if (e.pid1 == 11308 || e.pid2 == 11308) {
				//e.printOut();
				//}
			}
			sum += i;
			if (i != 0) {
				System.out.println(System.currentTimeMillis() + " Finish loading " + sum + " events.");
			}
			//FFang Adddd
			//break;
			//if (sum > nRead/26*10) break;
		}

		// Sort
		Collections.sort(elist);
		// Event test
		//for (Event e:elist) {
		//	e.printOut();
		//}
		//System.exit(0);

		starttime = elist.get(0).time;
		endtime = elist.get(elist.size()-1).time;
		long statETime = System.currentTimeMillis();
		System.out.println("Start time = " + starttime + ", End time = " + endtime);

		for (int pid : tidList) {
			if (createTimeList.containsKey(pid)) {
				prevList.put(pid, new prevState(-1, createTimeList.get(pid)));
			}
			else {
				prevList.put(pid, new prevState(-1, starttime));
			}

			long stime = starttime;
			long etime = endtime;
			if (createTimeList.containsKey(pid)) {
				stime = createTimeList.get(pid);
			}
			if (killTimeList.containsKey(pid)) {
				etime = killTimeList.get(pid);
			}
			overAllRes.get(pid).total = etime - stime;
		}

	}

	public class Event implements Comparable<Event>{
		long time;
		short type;
		short core;
		int pid1;
		int pid2;
		short irq;
		short pid1state;
		short pid2state;
		long perftime;

		public Event(ByteBuffer buf) throws IOException {
			this.type = buf.getShort();
			this.time = buf.getLong();
			this.core = buf.getShort();
			this.pid1 = buf.getInt();
			this.pid2 = buf.getInt();
			this.irq = buf.getShort();
			this.pid1state = buf.getShort();
			this.pid2state = buf.getShort();
			this.perftime = buf.getLong();
			//this.printOut();
			//buf.position(buf.position()+20);
			//this.f1 = buf.getLong();
			//this.f2 = buf.getInt();
			//this.f3 = buf.getInt();
			//this.f4 = buf.getInt();
		}

		@Override
		public int compareTo(Event e)
		{
			if (this.time > e.time) return 1;
			else if (this.time < e.time) return -1;
			else return 0;
		}

		public void printOut() {
			System.out.printf("%d %d %d %d %d %d %d %d %d\n", this.time, this.type, this.core, this.pid1, this.pid2, this.irq, this.pid1state, this.pid2state, this.perftime);
			//System.out.printf("%f %d %d %d %d %d %d %d\n", this.time, this.cmd, this.op, this.pid, this.f1, this.f2, this.f3, this.f4);
		}
	}

	public class SoftEvent implements Comparable<SoftEvent>{
		byte type;
		long stime;
		long etime;
		short core;

		public SoftEvent(ByteBuffer buf) throws IOException {
			this.type = buf.get();
			this.stime = buf.getLong();
			this.etime = buf.getLong();
			this.core = buf.getShort();
		}

		@Override
		public int compareTo(SoftEvent e)
		{
			if (this.stime > e.stime) return 1;
			else if (this.stime < e.stime) return -1;
			else return 0;
		}

		public void printOut() {
			System.out.printf("%d %d %d %d\n", this.type, this.stime, this.etime, this.core);
			//System.out.printf("%f %d %d %d %d %d %d %d\n", this.time, this.cmd, this.op, this.pid, this.f1, this.f2, this.f3, this.f4);
		}
	}

	public class GeneralRes {
		long running;
		long runnable;
		long wait;
		long hardirq;
		long softirq;
		long network;
		long disk;
		long other;
		long unknown;
		long total;
		public GeneralRes() {
			this.running = 0;
			this.runnable = 0;
			this.wait = 0;
			this.hardirq = 0;
			this.softirq = 0;
			this.network = 0;
			this.disk = 0;
			this.other = 0;
			this.unknown = 0;
			this.total = 0;
		}

		public void printOut(int pid) {
			System.out.printf("%d %f %f %f %f %f %f %f %f %f\n", pid, this.running/freq, this.runnable/freq, this.wait/freq, this.hardirq/freq, this.softirq/freq, this.network/freq, this.disk/freq, this.other/freq, this.unknown/freq);
			//System.out.printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", pid, this.running, this.runnable, this.wait, this.hardirq, this.softirq, this.network, this.disk, this.other, this.unknown);
		}

		public void printOut(int pid, PrintWriter fbreak) {
			fbreak.printf("%d %f %f %f %f %f %f %f %f %f %f\n", pid, this.running/freq, this.runnable/freq, this.wait/freq, this.hardirq/freq, this.softirq/freq, this.network/freq, this.disk/freq, this.other/freq, this.unknown/freq, this.total/freq);
			//System.out.printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", pid, this.running, this.runnable, this.wait, this.hardirq, this.softirq, this.network, this.disk, this.other, this.unknown);
		}
	}

	class WaitRange implements Comparable<WaitRange> {
		long stime = 0;
		long etime = 0;

		public WaitRange(long stime, long etime) {
			this.stime = stime;
			this.etime = etime;
		}

		@Override
		public int compareTo(WaitRange e)
		{
			if (this.stime > e.stime) return 1;
			else if (this.stime < e.stime) return -1;
			else return 0;
		}
	}

	class DPEvent implements Comparable<DPEvent> {
		long time = 0;
		int type = 0;

		public DPEvent(long time, int type) {
			this.time = time;
			this.type = type;
		}

		@Override
		public int compareTo(DPEvent e)
		{
			if (this.time > e.time) return 1;
			else if (this.time < e.time) return -1;
			else return 0;
		}

		public void printOut(int tid) {
			System.out.printf("tid = %d, time = %d, type = %d\n", tid, time, type);
		}
	}

	class Worker extends Thread {
		int id;
		Object cond = new Object();
		ArrayList<Integer> tidList = null;
		Index index = null;
		Segment seg = null;
		int op = 0;
		int isRunning = 1;

		// For stats collection
		long infiniteTime  = 0L;
		long processTime  = 0L;

		ConcurrentSkipListMap<Long, Segment> tm = null;
		HashMap<String, Long> hm = new HashMap<String, Long>();

		public Worker(int id, int op, Index index) {
			this.id = id;
			this.op = op;
			this.index = index;
		}

		public void run() {
			IndexResult irlist = new IndexResult();
			IndexResult ir = new IndexResult();
			while(isRunning == 1) {
				synchronized(cond) {
					while(op == 0) {
						try{
							System.out.printf("Thread %d starts to wait\n", id);
							cond.wait();
						}
						catch (InterruptedException e) {
						}
					}
					System.out.printf("Thread %d starts op %d\n", id, op);
				}
				// op == 0, find out the real wake up
				if (op == 1) {
					while(true) {
						if (index.getNext(ir) == null) {
							System.out.printf("Thread %d finishes op %d\n", id, op);
							op = 0;
							synchronized(index) {
								index.finishNum++;
								index.notifyAll();
							}
							break;
						}
						if (ir.seg.state == 2) {
							Segment newSeg = getRealWake(ir.seg, ir.tm);
							if (newSeg.endTime != ir.seg.endTime) {
								// Record new segment
								udsCheck.get(newSeg.pid).put(newSeg.startTime, newSeg);
								udsQuick.get(newSeg.pid).put(ir.seg.startTime, ir.seg);
							}
						}
					}
				}
				else if (op == 2) {
					int tid = 0;
					while(true) {
						tid = index.getNextTID();
						if (tid == -1) {
							op = 0;
							synchronized(index) {
								index.finishNum++;
								index.notifyAll();
							}
							System.out.printf("Thread %d finishes op %d\n", id, op);
							break;
						}
						System.out.printf("Thread %d starts tid %d for op %d\n", id, tid, op);
						ConcurrentSkipListMap<Long, Segment> cm = udsCheck.get(tid);
						if (cm.lastEntry() == null) {
							System.out.println("[Info] tid = " + tid);
						}
						else {
							seg = cm.lastEntry().getValue();
							Segment segPrev = null;
							while (cm.lowerEntry(seg.startTime) != null) {
								segPrev = cm.lowerEntry(seg.startTime).getValue();
								if (seg.endTime == segPrev.endTime) {
									cm.remove(seg.startTime);
								}
								seg = segPrev;
							}
						}
						System.out.printf("Thread %d finishes tid %d for op %d\n", id, tid, op);
					}
				}
				else if (op == 3) {
					LinkedList<Segment> tmpList = new LinkedList<Segment>();
					ArrayList<Integer> before = new ArrayList<Integer>();
					while(true) {
						if (index.getNextUDS(irlist) == null) {
							System.out.printf("Thread %d finishes for op %d\n", id, op);
							op = 0;
							synchronized(index) {
								index.finishNum++;
								index.notifyAll();
							}
							break;
						}


						if (isUDS == 1) {						
							// For fake wake-up
							for (Segment newSeg : irlist.list) {
								//System.out.printf("list size = %d\n", irlist.list.size());
								if (newSeg.state != 2) continue;
								ConcurrentSkipListMap<Long, Segment> udsMap = udsCheck.get(newSeg.pid);
								if (udsMap == null || udsMap.lowerEntry(newSeg.startTime) == null) {
									tmpList.add(newSeg);
								}
								else {
									//Segment tmp = udsMap.lowerEntry(newSeg.startTime).getValue();
									Segment tmp = udsMap.floorEntry(newSeg.startTime).getValue();
									if (tmp.endTime < newSeg.startTime || tmp.startTime == newSeg.startTime) {
										if (tmp.startTime == newSeg.startTime) {
											tmpList.add(tmp);
										}
										else {
											tmpList.add(newSeg);
										}
									}
									//if (tmp.endTime < newSeg.startTime) {
									//	tmpList.add(newSeg);
									//}
									//else if (tmp.startTime == newSeg.startTime) {
									//	tmpList.add(tmp);
									//}
									else {
									}
								}
							}
						}
						else {
							// For spin-lock, also works for other cases.
							for (Segment newSeg : irlist.list) {
								if (newSeg.state == 2) {
									newSeg.overlap.add(newSeg.pid);
									tmpList.add(newSeg);
								}
							}
							// Finish spin-lock
						}

						before.clear();
						while(!tmpList.isEmpty()) {
							Segment segTmp = tmpList.remove(0);

							//if (isUDS == 2) {
							//	// spinlock
							//	processTime += segTmp.endTime - segTmp.startTime;
							//	if (segTmp.pid > 0 && before.contains(segTmp.pid)) {
							//		infiniteTime += segTmp.endTime - segTmp.startTime;
							//		continue;
							//	}
							//	else if (segTmp.pid > 0) {
							//		before.add(segTmp.pid);
							//	}
							//	// end for spinlock 
							//}

							// If the temp segment's time range is wrong
							if (segTmp.startTime > segTmp.endTime)
								continue;

							if (segTmp.state == 2) {
								Segment sTmp = null;
								if (isUDS == 1) {
									// Check if the wake-up event is a fake one
									if (udsCheck.containsKey(segTmp.waitFor)) {
										try {
											if (!isFakeWakeNew(segTmp.waitFor, segTmp.endTime)) {
												sTmp = null;
											}
											else {
												sTmp = udsQuick.get(segTmp.waitFor).lowerEntry(segTmp.endTime).getValue();
											}
										}
										catch (Exception e) {
											sTmp = null;
											//continue;
										}
									}
								}

								// This means we cannot make sure if the waitFor thread is caused by a real or fake wake up.

								if (sTmp == null )
								{
									//if (segTmp.pid == 31234) {
									//	segTmp.printOut();
									//}
									addResult(hm, segTmp.pid, segTmp.waitFor, segTmp.startTime, segTmp.endTime);
									if (segTmp.waitFor <= 0 || segTmp.waitFor == segTmp.pid) {
										continue;
									}
									else {
										tmpList.addAll(findAll(segTmp.waitFor, segTmp.startTime, segTmp.endTime, segTmp.parent, segTmp, this));
									}
								}
								else {
									try {
										tmpList.add(new Segment(segTmp.pid, 2, segTmp.startTime, sTmp.endTime, sTmp.waitFor, segTmp.waitFor));
									}
									catch (Exception e) {
										System.out.println(e.getMessage());
										continue;
									}
								}
							}
						}
						irlist.list.clear();
					}
				}
				else if (op == 4) {
					int tid = 0;
					while(true) {
						tid = index.getNextTID();
						if (tid == -1) {
							op = 0;
							synchronized(index) {
								index.finishNum++;
								index.notifyAll();
							}
							System.out.printf("Thread %d finishes op %d\n", id, op);
							break;
						}

						System.out.printf("Thread %d starts tid %d for op %d\n", id, tid, op);

						ConcurrentSkipListMap<Long, Segment> cm = udsCheck.get(tid);
						ConcurrentSkipListMap<Long, Segment> tm = segmentMap.get(tid);

						//for (Segment s : cm.values()) {
						//	addResult(hm, s.pid, s.waitFor, s.startTime, s.endTime);
						//}
						//System.out.printf("Total cm size for thread %d is %d\n", tid, cm.size());

						//int tmpa = 0;
						for (Segment s : cm.values()) {
							//tmpa++;
							//if (tmpa % 100 == 0) {
							//	System.out.printf("op4 has worked %d for thread %d\n", tid, tmpa);
							//}
							// Prepare for new segment timestamp
							//System.out.printf("time = %d, tid = %d, wait pid = %d, start time = %d, end time = %d\n", System.currentTimeMillis(), s.pid, s.waitFor, s.startTime, s.endTime);
							if (tm.lowerEntry(s.startTime) == null) continue;
							Segment s1 = tm.lowerEntry(s.startTime).getValue();
							Segment s2 = null;
							Segment tmp = null;

							//if (s.parent == -99) {
							//	long tmpTime = s.endTime;
							//	do {
							//		tmp = tm.lowerEntry(tmpTime).getValue();
							//		tmpTime = tmp.startTime;
							//	} while(tmp.state != 2);
							//	s2 = tmp;
							//	//s.waitFor = s2.waitFor;
							//	s.endTime = s2.endTime;
							//}
							//else {
								s2 = tm.lowerEntry(s.endTime).getValue();
							//}
							long nstime = s1.startTime;
							long netime = s2.endTime;
							

							//s.printOut();
							if (s1.startTime == s2.startTime) {
								tm.remove(s1.startTime);
							}
							else {
								try {
									for (Segment stmp : tm.subMap(s1.startTime, true, s2.startTime, true).values()) {
										tm.remove(stmp.startTime);
									}
								}
								catch (Exception e) {
									System.out.println("op4 exception");
									continue;
								}
							}

							//if (s.parent == -99) {
							//	s.endTime = tmp.endTime;
							//	// Need add one more runnable state 
							//	tm.put(nstime, new Segment(tid, 0, nstime, s.startTime, 0, 0));
							//	tm.put(s.startTime, s);
							//}
							//else {
								tm.put(nstime, new Segment(tid, 0, nstime, s.startTime, 0, 0));
								tm.put(s.startTime, s);
								tm.put(s.endTime, new Segment(tid, 0, s.endTime, netime, 0, 0));
							//}
						}
					}
					System.out.printf("Thread %d finishes tid %d for op %d\n", id, tid, op);
				}
				else if (op == 5) {
					int tid = 0;
					while(true) {
						tid = index.getNextTID();
						if (tid == -1) {
							op = 0;
							synchronized(index) {
								index.finishNum++;
								index.notifyAll();
							}
							System.out.printf("Thread %d finishes op %d\n", id, op);
							break;
						}
						System.out.printf("Thread %d starts tid %d for op %d\n", id, tid, op);
						directBreakdown(elist, tid);
						System.out.printf("Thread %d finishes tid %d for op %d\n", id, tid, op);
					}
				}
			}
		}
	}

	class Index {
		int tid = 0;
		int tidpos = 0;
		long pos = 0;

		ArrayList<Integer> tidList = null;
		Map<Integer, ConcurrentSkipListMap<Long, Segment>> segmentMap = null;
		ConcurrentSkipListMap<Long, Segment> tm = null;
		ConcurrentSkipListMap<Long, Segment> udsMap = null;
		Iterator<Map.Entry<Long, Segment>> it = null;
		int nexttid = 0;
		int finishNum = 0;

		public void reset() {
			tid = 0;
			tidpos = 0;
			pos = 0;
			finishNum = 0;
			nexttid = 0;
			tm = null;
		}

		public Index() {
		}

		public Index(ArrayList<Integer> tidList, Map<Integer, ConcurrentSkipListMap<Long, Segment>> segmentMap) {
			this.tidList = tidList;
			this.segmentMap = segmentMap;
			this.tid = 0;
			this.tidpos = 0;
			this.pos = 0;
		}

		synchronized IndexResult getNextUDS(IndexResult ir) {
			int size = 1;
			// First run!
			if (tid == 0 || !it.hasNext()) {
				//System.out.printf("Worker %d finishes cascading on Thread %d, #rest = %d\n", Thread.currentThread().getId(), tid, tidList.size() - tidpos);
				if (tidpos >= tidList.size()) {
					return null;
				}
				else {
					tid = tidList.get(tidpos);
					tm = segmentMap.get(tid);
					it = tm.entrySet().iterator();
					tidpos++;
					System.out.printf("Worker %d starts cascading on Thread %d, #rest = %d\n", Thread.currentThread().getId(), tid, tidList.size() - tidpos);
				}
			}
			while (it.hasNext()) {
				ir.list.add(it.next().getValue());
				break;
				//if (ir.list.size() == size) {
				//	break;
				//}
			}
			return ir;
		}

		synchronized IndexResult getNext(IndexResult ir) {
			// First run!
			if (tid == 0 || !it.hasNext()) {
				System.out.printf("Worker %d finishes fakePre on Thread %d, #rest = %d\n", Thread.currentThread().getId(), tid, tidList.size() - tidpos);
				if (tidpos >= tidList.size()) {
					return null;
				}
				else {
					tid = tidList.get(tidpos);
					tm = segmentMap.get(tid);
					it = tm.entrySet().iterator();
					tidpos++;
					System.out.printf("Worker %d starts fakePre on Thread %d, #rest = %d\n", Thread.currentThread().getId(), tid, tidList.size() - tidpos);
				}
			}
			while (it.hasNext()) {
				ir.tm = tm;
				ir.seg = it.next().getValue();
				break;
			}
			return ir;
		}

		synchronized int getNextTID() {
			if (nexttid == tidList.size())
				return -1;
			return tidList.get(nexttid++);
		}
	}

	public Index createIndex(ArrayList<Integer> tidList, Map<Integer, ConcurrentSkipListMap<Long, Segment>> segmentMap) {
		return new Index(tidList, segmentMap);
	}

	class IndexResult {
		Segment seg = null;
		ArrayList<Segment> list = new ArrayList<Segment>();
		ConcurrentSkipListMap<Long, Segment> tm = null;

		public IndexResult() {
		}
	}

	void startWorker(ArrayList<Worker> workerList, Index index, int numWorker) {
		for (int i = 0; i < numWorker; i++) {
			workerList.add(new Worker(i, 0, index));
		}

		for (int i = 0; i < numWorker; i++) {
			workerList.get(i).start();
		}
	}

	public class SpinVector {
		int pid1;
		int pid2;
		long stime;
		long etime;
		
		public SpinVector() {
		}

		public void clear() {
			pid1 = 0;
			pid2 = 0;
			stime = 0;
			etime = 0;
		}
	}
}

