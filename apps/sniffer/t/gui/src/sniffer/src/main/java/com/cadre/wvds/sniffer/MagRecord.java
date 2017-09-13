package com.cadre.wvds.sniffer;

public class MagRecord {

	public long t;
	public int x;
	public int y;
	public int z;
	public int s;

	public MagRecord() { }

	public String toString() {
		return String.format("{%d,%d,%d,%d,%d}", t,x,y,z,s);
	}

}
