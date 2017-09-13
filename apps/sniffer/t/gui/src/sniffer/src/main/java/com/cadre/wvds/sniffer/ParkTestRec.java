package com.cadre.wvds.sniffer;

import java.util.Date;

import com.alibaba.fastjson.annotation.JSONField;

public class ParkTestRec {

	@JSONField (format="yyyy-MM-dd HH:mm:ss,SSS")
	public Date atime;
	public int nodeid;
	public int seqno;
	public int state;
	public int rssi;
	public int mag1x;
	public int mag1y;
	public int mag1z;
	public int mag2x;
	public int mag2y;
	public int mag2z;
	
	public ParkTestRec() { }
}
