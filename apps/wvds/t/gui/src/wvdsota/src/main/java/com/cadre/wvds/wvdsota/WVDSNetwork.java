package com.cadre.wvds.wvdsota;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.comm.CommAdapter;
import net.skaldchen.commons.utils.BytesUtil;

public class WVDSNetwork {

	private static final Logger log = LoggerFactory.getLogger(WVDSNetwork.class);

	private CommAdapter comm;

	public WVDSNetwork(CommAdapter comm) {
		this.comm = comm;
	}

	public void send(byte[] dest, int type, byte[] data) {
		if (dest == null || data == null)
			throw new RuntimeException("null dest or data");
		if (dest.length != 8)
			throw new RuntimeException("invalid dest");

		byte[] pkt = new byte[6 + 1 + data.length];
		System.arraycopy(dest, 2, pkt, 0, 6);
		pkt[6] = (byte)type;
		System.arraycopy(data, 0, pkt, 7, data.length);

		try {
			log.info(String.format("send data: (%d) %s", pkt.length, BytesUtil.strBytes(pkt)));
			byte[] enc = WVDS.encrypt(pkt, WVDS.aesKey, WVDS.aesIV);
			log.debug(String.format("send: (%d) %s", enc.length, BytesUtil.strBytes(enc)));
			comm.send(enc);
		} catch (Exception e) {
			log.warn("fail send", e);
		}
	}

	public void getInfo(byte[] target, int type) {
		byte[] resp = new byte[7];
		int i = 0;
		System.arraycopy(WVDS.getTS(), 0, resp, i, 6); i += 6;
		resp[i++] = (byte) type;
		log.info(String.format("get info of node %s", Mote.strMAC(target)));
		send(target, (WVDS.PKT_NODEINFO | 0x80), resp);
	}

	public void resetMote(byte[] target) {
		byte[] resp = new byte[6];
		int i = 0;
		System.arraycopy(WVDS.getTS(), 0, resp, i, 6); i += 6;
		log.info(String.format("reset node %s", Mote.strMAC(target)));
		send(target, (WVDS.PKT_RESET | 0x80), resp);
	}

	public void execOTA(int type, int version, byte[] target, int mode, int count, int autop, byte[] commit) {
		byte[] resp = new byte[27];
		byte[] mac = new byte[8];

		log.info(String.format("exec OTA ver 0x%02X to type %d, mode %d, target %s, commit %s", version, type, mode, Mote.strMAC(target), new String(commit)));
		if (mac != null) {
			int i = 0;
			System.arraycopy(WVDS.getTS(), 0, resp, i, 6); i += 6;
			resp[i++] = (byte) type;
			resp[i++] = (byte) version;
			System.arraycopy(target, 0, resp, i, 8); i += 8;
			resp[i++] = (byte) mode;
			resp[i++] = (byte) count;
			resp[i++] = (byte) autop;
			System.arraycopy(commit, 0, resp, i, 8); i += 8;
			send(mac, (WVDS.PKT_OTAEXEC | 0x80), resp);
		}
	}

	public void setRadio(int chan, int power) {
		byte[] resp = new byte[13];
		byte[] mac = new byte[8];
		int freq = (906 + chan * 2) * 1000000;

		log.info(String.format("set radio chan %d power %d", chan, power));
		if (mac != null) {
			int i = 0;
			System.arraycopy(WVDS.getTS(), 0, resp, i, 6); i += 6;
			resp[i++] = (byte) WVDS.SET;
			resp[i++] = (byte) chan;
			BytesUtil.putBEUInt32(freq, resp, i); i += 4;
			resp[i++] = (byte) power;
			send(mac, (WVDS.PKT_RADIO | 0x80), resp);
		}
	}

	public void freset() {
		String cmd = "AT+FRESET\r\n";
		byte[] pkt = cmd.getBytes();
		try {
			log.info("freset ota node");
			comm.send(pkt);
		} catch(Exception e) {
			log.warn("fail freset", e);
		}
	}
}
