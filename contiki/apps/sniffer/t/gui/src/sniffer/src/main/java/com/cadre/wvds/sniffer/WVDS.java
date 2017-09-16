package com.cadre.wvds.sniffer;

import java.util.Calendar;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class WVDS {

    private static final Logger log = LoggerFactory.getLogger(WVDS.class);

    public static final byte VD = 0x01;
    public static final byte RP = 0x02;
    public static final byte AP = 0x04;
    public static byte[] TYPE = new byte[]{AP,RP,VD};
    public static String[] NAME = new String[]{"N/A", "VD", "RP", "N/A", "AP"};

    public static final byte OTA_ONENODE = 0x01;
    public static final byte OTA_ONEHOP  = 0x02;
    public static final byte OTA_ALLNODE = 0x03;

    public static final byte PKT_BEG = (byte) 0xAA;
    public static final byte PKT_END = (byte) 0xFF;

    public static final int PKT_PARKEVT = 0x01;
    public static final int PKT_VDHBEAT = 0x02;
    public static final int PKT_RPHBEAT = 0x03;
    public static final int PKT_APHBEAT = 0x04;
    public static final int PKT_APTSYNC = 0x05;
    public static final int PKT_APCONN  = 0x11;
    public static final int PKT_NODETBL = 0x12;
    public static final int PKT_BLACKL  = 0x13;
    public static final int PKT_GPRS    = 0x14;
    public static final int PKT_RADIO   = 0x15;
    public static final int PKT_STATE   = 0x16;
    public static final int PKT_LOCKOR  = 0x17;
    public static final int PKT_OTADATA = 0x18;
    public static final int PKT_OTAEXEC = 0x19;
    public static final int PKT_VDCONN  = 0x21;
    public static final int PKT_ALARM   = 0x22;
    public static final int PKT_VERSION = 0x23;
    public static final int PKT_MAGDATA = 0x24;
    public static final int PKT_BASIC   = 0x25;
    public static final int PKT_ALGO    = 0x26;
    public static final int PKT_BASEL   = 0x27;
    public static final int PKT_RESET   = 0x31;
    public static final int PKT_FRESET  = 0x32;
    public static final int PKT_VDACT   = 0x33;
    public static final int PKT_FAECONN = 0x34;
    public static final int PKT_DISCONN = 0x35;
    public static final int PKT_REINIT  = 0x36;

    public static final int OP_MASK = 0x3F;

    public static final int GET = 0x01;
    public static final int SET = 0x02;
    public static final int ADD = 0x03;
    public static final int DEL = 0x04;

    public static final int UNLOCK = 0x00;
    public static final int LOCK   = 0x01;

    public static String strDev(byte[] dev) {
        StringBuilder sb = new StringBuilder();
        for (byte b : dev) {
            sb.append(String.format("%02X", b));
        }
        return sb.toString();
    }

    public static String strVer(int ver) {
        return String.format("v%d.%d", (ver >> 4), (ver & 0x0f));
    }

    public static String strState(int s) {
		final String[] NAME = new String[]{"无车/车离开","有车/车驶入","等待激活","标定中/初始化"};
		return ((s < NAME.length) ? NAME[s] : "N/A");
    }

    public static String typeName(int type) {
        type = (type & 0xBF);
        if (type == 0x01) return "0x01 ParkEvt";
        if (type == 0x81) return "0x81 ParkEvt ACK";
        if (type == 0x02) return "0x02 VDHbeat";
        if (type == 0x82) return "0x82 VDHbeat ACK";
        if (type == 0x03) return "0x03 RPHbeat";
        if (type == 0x83) return "0x83 RPHbeat ACK";
        if (type == 0x04) return "0x04 APHbeat";
        if (type == 0x84) return "0x84 APHbeat ACK";
        if (type == 0x05) return "0x05 APTSync";
        if (type == 0x85) return "0x85 APTSync ACK";
        if (type == 0x11) return "0x11 APConn";
        if (type == 0x91) return "0x91 APConn ACK";
        if (type == 0x16) return "0x16 On/Offline";
        if (type == 0x21) return "0x21 VDConn";
        if (type == 0xA1) return "0xA1 VDConn ACK";
        if (type == 0x22) return "0x22 Alarm";
        if (type == 0xA2) return "0xA2 Alarm ACK";
        if (type == 0x93) return "0x93 Blacklist";
        if (type == 0x13) return "0x13 Blacklist ACK";
        if (type == 0x94) return "0x94 GPRSParam";
        if (type == 0x14) return "0x14 GPRSParam ACK";
        if (type == 0x95) return "0x95 RadioParam";
        if (type == 0x15) return "0x15 RadioParam ACK";
        if (type == 0x98) return "0x98 OTAData";
        if (type == 0x18) return "0x98 OTAData ACK";
        if (type == 0x99) return "0x99 OTAExec";
        if (type == 0x19) return "0x19 OTAExec ACK";
        if (type == 0x24) return "0x24 Magnetic";
        if (type == 0xA4) return "0xA4 Magnetic ACK";
        if (type == 0xA5) return "0xA5 BasicParam";
        if (type == 0x25) return "0x25 BasicParam ACK";
        if (type == 0xA6) return "0xA6 AlgoParam";
        if (type == 0x26) return "0x26 AlgoParam ACK";
        if (type == 0xB1) return "0xB1 Reset";
        if (type == 0x31) return "0x31 Reset ACK";
        if (type == 0xB2) return "0xB2 FReset";
        if (type == 0x32) return "0x32 FReset ACK";
        if (type == 0xB6) return "0xB6 VDReinit";
        if (type == 0x36) return "0x36 VDReinit ACK";
        return "N/A";
    }

    public static byte[] dev2mac(byte[] dev) {
        byte[] mac = new byte[8];
        System.arraycopy(dev, 0, mac, 2, dev.length);
        return mac;
    }

    public static int dev2addr(byte[] dev) {
        return ((dev[0] & 0xff) << 8) + (dev[5] & 0xff);
    }

    // 0x12 -> 12
    private static int deBCD(byte n) {
        return ((n & 0xF0) >> 4) * 10 + (n & 0x0F);
    }

    public static long time(byte[] ts) {
        Calendar cal = Calendar.getInstance();
        cal.set(Calendar.YEAR, 2000 + deBCD(ts[0]));
        cal.set(Calendar.MONTH, deBCD(ts[1]) - 1);
        cal.set(Calendar.DAY_OF_MONTH, deBCD(ts[2]));
        cal.set(Calendar.HOUR_OF_DAY, deBCD(ts[3]));
        cal.set(Calendar.MINUTE, deBCD(ts[4]));
        cal.set(Calendar.SECOND, deBCD(ts[5]));
        cal.set(Calendar.MILLISECOND, 0);
        return cal.getTimeInMillis();
    }

    // 12 -> 0x12
    public static byte BCD(int b) {
        return (byte)(((b / 10) << 4) + (b % 10));
    }

    public static byte[] getTS() {
        byte[] ts = new byte[6];
        Calendar cal = Calendar.getInstance();
        ts[0] = BCD((cal.get(Calendar.YEAR) - 2000));
        ts[1] = BCD(cal.get(Calendar.MONTH) + 1);
        ts[2] = BCD(cal.get(Calendar.DAY_OF_MONTH));
        ts[3] = BCD(cal.get(Calendar.HOUR_OF_DAY));
        ts[4] = BCD(cal.get(Calendar.MINUTE));
        ts[5] = BCD(cal.get(Calendar.SECOND));
        return ts;
    }

}
