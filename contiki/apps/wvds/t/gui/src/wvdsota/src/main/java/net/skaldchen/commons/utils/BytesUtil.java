package net.skaldchen.commons.utils;

public class BytesUtil {

    public static String strBytes(byte[] bytes) {
        return strBytes(bytes, 0, bytes.length);
    }

    public static String strBytes(byte[] bytes, int ofs, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++)
            sb.append(String.format("%02X ", bytes[ofs + i]));
        return (sb.length() > 1) ? sb.substring(0, sb.length() - 1) : "";
    }

    public static String txtBytes(byte[] bytes, int ofs, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            if (bytes[ofs+i] == 0x0D) {
                sb.append("\\r");
            } else if (bytes[ofs+i] == 0x0A) {
                sb.append("\\n");
            } else {
                sb.append((char)bytes[ofs+i]);
            }
        }
        return sb.toString();
    }

    public static byte[] toBytes(String str) {
        return toBytes(str, " ");
    }

    public static byte[] toBytes(String str, String sep) {
        String[] subs = str.split(sep);
        byte[] data = new byte[subs.length];
        for (int i = 0; i < data.length; i++) {
            data[i] = (byte)Integer.parseInt(subs[i],16);
        }
        return data;
    }

    public static byte[] getBytes(byte[] data, int ofs, int len) {
        byte[] out = new byte[len];
        System.arraycopy(data, ofs, out, 0, len);
        return out;
    }

    public static boolean equal(byte[] a, byte[] b) {
        if (a.length != b.length)
            return false;
        for (int i = 0; i < a.length; i++) {
            if (a[i] != b[i])
                return false;
        }
        return true;
    }

    public static int getUInt8(byte[] data, int ofs) {
        return (data[ofs] & 0xff);
    }

    public static int getInt8(byte[] data, int ofs) {
        int val = (data[ofs] & 0xff);
        if ((val & 0x80) > 0)
            val = -(0xff - val + 1);
        return val;
    }

    public static int getLEUInt16(byte[] data, int ofs) {
        return (data[ofs] & 0xff) + ((data[ofs+1] & 0xff) << 8);
    }

    public static int getBEUInt16(byte[] data, int ofs) {
        return ((data[ofs] & 0xff) << 8) + ((data[ofs+1] & 0xff));
    }

    public static int getBEInt16(byte[] data, int ofs) {
        int val = ((data[ofs] & 0xff) << 8) + ((data[ofs+1] & 0xff));
        if ((val & 0x8000) > 0) val = -65536 + val;
        return val;
    }

    public static int getLEUInt32(byte[] data, int ofs) {
        return ((data[ofs] & 0xff) << 0) + ((data[ofs+1] & 0xff) << 8) +
            ((data[ofs+2] & 0xff) << 16) + ((data[ofs+3] & 0xff) << 24);
    }

    public static int getBEUInt32(byte[] data, int ofs) {
        return ((data[ofs] & 0xff) << 24) + ((data[ofs+1] & 0xff) << 16) +
            ((data[ofs+2] & 0xff) << 8) + ((data[ofs+3] & 0xff) << 0);
    }

    public static int getBCD(byte[] data, int ofs) {
        return getBCD(data, ofs, 1);
    }

    public static int getBCD(byte[] data, int ofs, int len) {
        int num = 0;
        for (int i = 0; i < len; i++) {
            int h = ((data[ofs + i] & 0xf0) >> 4);
            int l = (data[ofs + i] & 0x0f);
            num = num * 10 + h;
            num = num * 10 + l;
        }
        return num;
    }

    public static void putUInt8(int val, byte[] data, int ofs) {
        data[ofs] = (byte)val;
    }

    public static void putInt8(int val, byte[] data, int ofs) {
        data[ofs] = (byte)(val > 0 ? val : (256+val));
    }

    public static void putLEUInt16(int val, byte[] data, int ofs) {
        data[ofs + 0] = (byte) ((val >>  0) & 0xff);
        data[ofs + 1] = (byte) ((val >>  8) & 0xff);
    }

    public static void putBEUInt16(int val, byte[] data, int ofs) {
        data[ofs + 0] = (byte) ((val >>  8) & 0xff);
        data[ofs + 1] = (byte) ((val >>  0) & 0xff);
    }

    public static void putBEInt16(int val, byte[] data, int ofs) {
        data[ofs + 0] = (byte) (((val & 0xffff) >>  8) & 0xff);
        data[ofs + 1] = (byte) (((val & 0xffff) >>  0) & 0xff);
    }

    public static void putLEUInt32(int val, byte[] data, int ofs) {
        data[ofs + 0] = (byte) ((val >>  0) & 0xff);
        data[ofs + 1] = (byte) ((val >>  8) & 0xff);
        data[ofs + 2] = (byte) ((val >> 16) & 0xff);
        data[ofs + 3] = (byte) ((val >> 24) & 0xff);
    }

    public static void putBEUInt32(int val, byte[] data, int ofs) {
        data[ofs + 0] = (byte) ((val >> 24) & 0xff);
        data[ofs + 1] = (byte) ((val >> 16) & 0xff);
        data[ofs + 2] = (byte) ((val >>  8) & 0xff);
        data[ofs + 3] = (byte) ((val >>  0) & 0xff);
    }

    public static String getString(byte[] data, int ofs) {
        StringBuffer sb = new StringBuffer();
        for (int i = ofs; i < data.length && data[i] != 0; i++)
            sb.append((char)data[i]);
        return sb.toString();
    }

    public static String getString(byte[] arr, int ofs, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = ofs; i < arr.length && i < (ofs + len); i++) {
            if (arr[i] == 0)
                break;
            sb.append((char)arr[i]);
        }
        return sb.toString();
    }
}
