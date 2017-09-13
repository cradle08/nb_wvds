package com.cadre.wvds.wvdsota;

import net.skaldchen.commons.utils.CRC16;

public class OTAFrame {

	public int role;   // uint8_t
	public int ver;    // uint8_t
	public int begfin; // uint8_t
	public int reserv; // uint8_t
	public int addr;   // uint32_t
	public int len;    // uint16_t
	public byte[] data;
	public int crc;    // uint16_t

	public static final int DATALEN = 128;

	public OTAFrame() { }

	public OTAFrame(byte[] frame) {
		if (frame.length != 12 + DATALEN)
			throw new RuntimeException("not valid ota frame");
		int i = 0;
		role = (frame[i++] & 0xff);
		ver  = (frame[i++] & 0xff);
		begfin = (frame[i++] & 0xff);
		reserv = (frame[i++] & 0xff);
		addr = ((frame[i] & 0xff)<<24) + ((frame[i+1] & 0xff)<<16)
				+ ((frame[i+2] & 0xff)<<8) + ((frame[i+3] & 0xff)<<0); i += 4;
		len = ((frame[i] & 0xff)<<8) + ((frame[i+1] & 0xff)<<0); i += 2;
		for (int j = 0; j < DATALEN; j++)
			data[j] = frame[i++];
		crc = ((frame[i] & 0xff)<<8) + ((frame[i+1] & 0xff)<<0); i += 2;
	}

	public byte[] toBytes() {
		byte[] bytes = new byte[12 + DATALEN];
		int i = 0;
		bytes[i++] = (byte)role;
		bytes[i++] = (byte)ver;
		bytes[i++] = (byte)begfin;
		bytes[i++] = (byte)reserv;
		bytes[i++] = (byte)((addr>>24) & 0xff);
		bytes[i++] = (byte)((addr>>16) & 0xff);
		bytes[i++] = (byte)((addr>> 8) & 0xff);
		bytes[i++] = (byte)((addr>> 0) & 0xff);
		bytes[i++] = (byte)((len >> 8) & 0xff);
		bytes[i++] = (byte)((len >> 0) & 0xff);
		for (int j = 0; j < DATALEN; j++)
			bytes[i++] = data[j];
		int crc = CRC16.calc(0, bytes, 0, i);
		bytes[i++] = (byte)((crc >> 8) & 0xff);
		bytes[i++] = (byte)((crc >> 0) & 0xff);
		return bytes;
	}

}
