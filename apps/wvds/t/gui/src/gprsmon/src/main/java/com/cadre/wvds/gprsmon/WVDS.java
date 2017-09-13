package com.cadre.wvds.gprsmon;

import java.util.Base64;
import java.util.Calendar;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.util.AES;
import net.skaldchen.commons.util.BytesUtil;
import net.skaldchen.commons.util.CRC16;

public class WVDS {

    private static final Logger log = LoggerFactory.getLogger(WVDS.class);

    public static final byte VD = 0x01;
    public static final byte RP = 0x02;
    public static final byte AP = 0x04;
    public static byte[] TYPE = new byte[]{AP,RP,VD};
    public static String[] NAME = new String[]{"NA", "VD", "RP", "NA", "AP"};

    public static final byte OTA_ONENODE = 0x01;
    public static final byte OTA_ONEHOP  = 0x02;
    public static final byte OTA_ALLNODE = 0x03;

    public static final int PKT_PARKEVT = 0x01;
    public static final int PKT_VDHBEAT = 0x02;
    public static final int PKT_RPHBEAT = 0x03;
    public static final int PKT_APHBEAT = 0x04;
    public static final int PKT_APTSYNC = 0x05;
    public static final int PKT_APCONN  = 0x11;
    public static final int PKT_BLACKL  = 0x13;
    public static final int PKT_GPRS    = 0x14;
    public static final int PKT_RADIO   = 0x15;
    public static final int PKT_STATE   = 0x16;
    public static final int PKT_LOCKOR  = 0x17;
    public static final int PKT_FWPULL  = 0x18;
    public static final int PKT_FWEXEC  = 0x19;
    public static final int PKT_VDCONN  = 0x21;
    public static final int PKT_ALARM   = 0x22;
    public static final int PKT_VERSION = 0x23;
    public static final int PKT_MAGDATA = 0x24;
    public static final int PKT_BASIC   = 0x25;
    public static final int PKT_ALGO    = 0x26;
    public static final int PKT_BASEL   = 0x27;

    public static final int OP_MASK = 0x3F;

    public static final int GET = 0x01;
    public static final int SET = 0x02;
    public static final int ADD = 0x03;
    public static final int DEL = 0x04;

    public static String strDev(byte[] dev) {
        StringBuilder sb = new StringBuilder();
        for (byte b : dev) {
            sb.append(String.format("%02X", b));
        }
        return sb.toString();
    }

    public static String strTS(byte[] ts) {
        if (ts == null)
            return "null";
        StringBuilder sb = new StringBuilder();
        for (byte b : ts) {
            sb.append(String.format("%02X", b));
        }
        return sb.toString();
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
        if (type == 0x99) return "0x99 OTAExec";
        if (type == 0x19) return "0x19 OTAExec ACK";
        if (type == 0xA5) return "0xA5 BasicParam";
        if (type == 0x25) return "0x25 BasicParam ACK";
        if (type == 0x24) return "0x24 Magnetic";
        if (type == 0xA4) return "0xA4 Magnetic ACK";
        if (type == 0xB0) return "0xB0 Reset";
        if (type == 0x30) return "0x30 Reset ACK";
        return String.format("0x%02X", type);
    }

    public static byte[] dev2mac(byte[] dev) {
        byte[] mac = new byte[8];
        System.arraycopy(dev, 0, mac, 2, dev.length);
        return mac;
    }

    public static int dev2addr(byte[] dev) {
        return ((dev[0] & 0xff) << 8) + (dev[5] & 0xff);
    }

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

    public static byte[] decrypt(byte[] enc, String key, String iv) throws Exception {
        if (enc[0] != (byte)0xAA)
            throw new RuntimeException(String.format("invalid head byte 0x%02X", enc[0]));
        if (enc[enc.length-1] != (byte)0xFF)
            throw new RuntimeException(String.format("invalid tail byte 0x%02X", enc[enc.length-1]));
        if (enc.length != (5 + (enc[1]&0xff)))
            throw new RuntimeException(String.format("invalid length 0x%02X", enc[1]));

        byte[] data = BytesUtil.getBytes(enc, 2, enc.length-5);
        byte[] dec1 = Base64.getDecoder().decode(data);
        byte[] dec2 = AES.decrypt(dec1, key, iv);
        byte[] dec3 = BytesUtil.getBytes(dec2, 0, dec2.length - dec2[dec2.length-1]);
        return dec3;
    }

    public static byte[] encrypt(byte[] data, String key, String iv) throws Exception {
        byte[] enc1 = AES.encrypt(data, key, iv);
        byte[] enc2 = Base64.getEncoder().encode(enc1);
        byte[] enc3 = new byte[enc2.length + 5];
        int i = 0;
        enc3[i++] = (byte) 0xAA;
        enc3[i++] = escapeLen((byte)enc2.length);
        System.arraycopy(enc2, 0, enc3, 2, enc2.length); i += enc2.length;
        int ccrc = CRC16.calc(0x0000, enc3, 1, 1 + enc2.length);
        enc3[i++] = escapeCrc((byte)(ccrc >> 8));
        enc3[i++] = escapeCrc((byte)(ccrc & 0xff));
        enc3[i++] = (byte) 0xFF;
        return enc3;
    }

    private static byte escapeLen(byte b) {
        if (b == (byte)0xFF)
            return 0x01;
        if (b == (byte)0xAA)
            return 0x02;
        return b;
    }

    private static byte escapeCrc(byte b) {
        if (b == (byte)0xAA)
            return 0x01;
        if (b == (byte)0xFF)
            return (byte)0xFE;
        return b;
    }
}
