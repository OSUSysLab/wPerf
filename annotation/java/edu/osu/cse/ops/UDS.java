package edu.osu.cse.ops;

public class UDS {
	static {
		System.load("/tmp/libkerntool.so");
		init();
	}

	public native static void add(long address, int type);
	public native static void init();

	public UDS() {
	}

}
