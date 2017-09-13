package com.cadre.wvds.wvdsota;

public class Mote {

	private byte[] mac;
	private int type;
	private int ver;
	private int netId;

	public boolean otaPending = false;

	public Mote() { }

	public byte[] getMac() {
		return mac;
	}

	public void setMac(byte[] mac) {
		this.mac = mac;
	}

	public int getType() {
		return type;
	}

	public void setType(int type) {
		this.type = type;
	}

	public int getVer() {
		return ver;
	}

	public void setVer(int ver) {
		this.ver = ver;
	}

	public int getNetId() {
		return netId;
	}

	public void setNetId(int id) {
		this.netId = id;
	}

	public static String strMAC(byte[] mac, String sep) {
		StringBuilder sb = new StringBuilder();
		for (byte b : mac) {
			sb.append(String.format("%02X", b)).append(sep);
		}
		return (sb.length() > 0 ? sb.substring(0, sb.length()-sep.length()) : "");
	}

	public static String strMAC(byte[] mac) {
		StringBuilder sb = new StringBuilder();
		for (byte b : mac) {
			sb.append(String.format("%02X ", b));
		}
		return (sb.length() > 0 ? sb.substring(0, sb.length()-1) : "");
	}

	public static byte[] toMAC(String str) {
		String[] subs = str.split(" ");
		byte[] mac = new byte[subs.length];
		for (int i = 0; i < mac.length; i++) {
			mac[i] = (byte)Integer.parseInt(subs[i], 16);
		}
		return mac;
	}

}
