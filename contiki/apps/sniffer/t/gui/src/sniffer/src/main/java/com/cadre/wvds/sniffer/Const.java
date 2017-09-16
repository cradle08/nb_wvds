package com.cadre.wvds.sniffer;

public class Const {

    public static final int SNIFFER_ADDR = 65533;

    public static final short MOTE_CMD_NONE    = 0;
    public static final short MOTE_CMD_RESET   = 1;
    public static final short MOTE_CMD_START   = 2;
    public static final short MOTE_CMD_STOP    = 3;
    public static final short MOTE_CMD_SLEEP   = 4;
    public static final short MOTE_CMD_WAKEUP  = 5;
    public static final short MOTE_CMD_REROUTE = 6;
    public static final short MOTE_CMD_CTPTEST = 7;
    public static final short MOTE_CMD_TEST    = 8;

    public static final short BASE_CMD_RESET = 0;
    public static final short BASE_CMD_QUERYIDENT = 0x20;

    public static final short MOTE_ANY = 3;

    public static final short CMD_BASE_KEY = 0;

    public static final byte SNIFFER_CMD_GETVER  = 0x00;
    public static final byte SNIFFER_CMD_SETCHAN = 0x01;
    public static final byte SNIFFER_CMD_GETCHAN = 0x02;
    public static final byte SNIFFER_CMD_SENDPKT = 0x03;
    public static final byte SNIFFER_CMD_SETPWR = 0x05;
    public static final byte SNIFFER_CMD_GETPWR = 0x06;
    public static final byte SNIFFER_CMD_SEARCH  = 0x10;
    public static final byte SNIFFER_CMD_TESTRX  = 0x11;

    public static final byte SNIFFER_DATA_VER    = 0x00;
    public static final byte SNIFFER_DATA_START  = 0x01;
    public static final byte SNIFFER_DATA_CHAN   = 0x02;
    public static final byte SNIFFER_DATA_PWR   = 0x03;
    public static final byte SNIFFER_DATA_ERROR  = 0x11;

    public static final int TRACER_NUM_PER_MSG = 10;

    public static final byte SNIFFER_MSG  = 1;
    public static final byte SNIFFER_DATA = 2;
    public static final byte SNIFFER_CMD  = 3;
    public static final byte SNIFFER_ACK  = 4;

}
