package net.skaldchen.commons.util;

public class CRC16 {

    public static int calc(int crc0, byte[] data, int ofs, int len) {
        int crc = crc0;
        for (int i = 0; i < len; i++) {
            crc = add(crc, data[ofs+i]);
        }
        return crc;
    }

    public static int calc(int crc0, byte[] data) {
        return calc(crc0, data, 0, data.length);
    }

    public static int add(int crc, byte b) {
        crc = (crc & 0xffff) ^ (b & 0xff);
        crc = ((crc & 0xffff) >> 8) | ((crc & 0xffff) << 8);
        crc = (crc & 0xffff) ^ ((crc & 0xff00) << 4);
        crc = (crc & 0xffff) ^ ((crc & 0xffff) >> 12);
        crc = (crc & 0xffff) ^ ((crc & 0xff00) >> 5);
        return crc;
    }

}
