package net.skaldchen.commons.utils;

public class CRC16 {

    public static int calc(byte[] data, int crc0) {
        int crc = crc0;
        for (byte b : data)
            crc = add(crc, b);
        return crc;
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
