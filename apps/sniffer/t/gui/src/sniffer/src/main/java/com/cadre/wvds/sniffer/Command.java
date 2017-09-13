package com.cadre.wvds.sniffer;

public class Command {

	public static int CMD_BASE_KEY = 0;
	public static int CMD_APP_KEY  = 1;
	
	/* base.h */
	public static final int BASE_CMD_RESET      = 0x00;
	public static final int BASE_CMD_SLEEP      = 0x01;
	public static final int BASE_CMD_WAKEUP     = 0x02;
	public static final int BASE_CMD_ACCEPT     = 0x03;
	public static final int BASE_CMD_REJECT     = 0x04;
	public static final int BASE_CMD_FIND       = 0x05;
	public static final int BASE_CMD_FIND_RESET = 0x06;
	public static final int BASE_CMD_SETIDENT   = 0x10;
	public static final int BASE_CMD_SETLONGID  = 0x11;
	public static final int BASE_CMD_SETSHORTID = 0x12;
	public static final int BASE_CMD_SETPANID   = 0x13;
	public static final int BASE_CMD_SETROLE    = 0x14;
	public static final int BASE_CMD_SETFREQ    = 0x15;
	//public static final int BASE_CMD_QUERY      = 0x20;
	public static final int BASE_CMD_QUERYIDENT = 0x20;
	public static final int BASE_CMD_QUERYMETA  = 0x21;
	public static final int BASE_CMD_QUERYCHIP  = 0x24;
	public static final int BASE_CMD_QUERYSTAT  = 0x23;
	public static final int BASE_CMD_QUERYNBRS  = 0x22;

	/* app.h */
	public static final int APP_CMD_POWEROFF = 0;
	public static final int APP_CMD_POWERON  = 1;
	public static final int APP_CMD_CALIBRATE = 2;
	public static final int APP_CMD_CALENERGY = 3;
	public static final int APP_CMD_ADJUSTRATE = 4;
	public static final int APP_CMD_ADJUSTZERO = 5;
	public static final int APP_CMD_READCAL = 6;
	public static final int APP_CMD_QUERYDATA = 7;
	public static final int APP_CMD_QUERYCONF = 8;
	public static final int APP_CMD_QUERY = 0x10;

	public static String getCmdOp(int cid, int op) {
		String opstr = String.format("0x%02X", op);
		if (cid == CMD_BASE_KEY) {
			if (op == BASE_CMD_RESET) opstr = "reset";
			if (op == BASE_CMD_SLEEP) opstr = "sleep";
			if (op == BASE_CMD_WAKEUP) opstr = "wakeup";
			if (op == BASE_CMD_ACCEPT) opstr = "accept";
			if (op == BASE_CMD_REJECT) opstr = "reject";
			if (op == BASE_CMD_FIND) opstr = "find";
			if (op == BASE_CMD_FIND_RESET) opstr = "find&reset";
			if (op == BASE_CMD_SETIDENT) opstr = "setident";
			if (op == BASE_CMD_SETLONGID) opstr = "setladdr";
			if (op == BASE_CMD_SETSHORTID) opstr = "setsaddr";
			if (op == BASE_CMD_SETPANID) opstr = "setpanid";
			if (op == BASE_CMD_SETROLE) opstr = "setrole";
			if (op == BASE_CMD_SETFREQ) opstr = "setfreq";
			//if (op == BASE_CMD_QUERY) opstr = "query";
			if (op == BASE_CMD_QUERYIDENT) opstr = "queryident";
			if (op == BASE_CMD_QUERYMETA) opstr = "querymeta";
			if (op == BASE_CMD_QUERYCHIP) opstr = "querychip";
			if (op == BASE_CMD_QUERYSTAT) opstr = "querystat";
			if (op == BASE_CMD_QUERYNBRS) opstr = "querynbrs";
		}
		if (cid == CMD_APP_KEY) {
			if (op == APP_CMD_POWEROFF) opstr = "poweroff";
			if (op == APP_CMD_POWERON) opstr = "poweron";
			if (op == APP_CMD_CALIBRATE) opstr = "calibrate";
			if (op == APP_CMD_CALENERGY) opstr = "calenergy";
			if (op == APP_CMD_ADJUSTRATE) opstr = "adjustrate";
			if (op == APP_CMD_ADJUSTZERO) opstr = "adjustzero";
			if (op == APP_CMD_READCAL) opstr = "readcal";
			if (op == APP_CMD_QUERYDATA) opstr = "querydata";
			if (op == APP_CMD_QUERYCONF) opstr = "queryconf";
			if (op == APP_CMD_QUERY) opstr = "query";
		}
		return opstr;
	}
	
}
