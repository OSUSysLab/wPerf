package edu.osu.cse.ops;

import java.io.Serializable; 

public class UDSResult implements Serializable{
	long ts;
	int pid;
	int type;
	public UDSResult(long ts, int pid, int type) {
		this.ts = ts;
		this.pid = pid;
		this.type = type;
	}

	public void printOut() {
		System.out.printf("[Fang-UDS] Time %d ThreadID %d Type %d\n", ts, pid, type);
	}

	public long getTs() {
		return ts;
	}

	public void setTs(long ts) {
		this.ts = ts;
	}

	public int getPid() {
		return pid;
	}

	public void setPid(int pid) {
		this.pid = pid;
	}

	public int getType() {
		return type;
	}

	public void setType(int type) {
		this.type = type;
	}
}
