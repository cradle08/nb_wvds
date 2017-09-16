package com.cadre.wvds.sniffer;

public class Mote {

	private String name;      // room
	private int seqno;        // module seqno
	private int saddr;        // in-network short address
	private byte[] mac;
	private byte[] dev;  // meter 6-byte No.

	public boolean conned = false;
	public int eventN = 0;

	public Mote() {
		this.name = "N/A";
		this.saddr = 65534;
		this.seqno = -1;
		this.dev = new byte[6];
		this.mac = new byte[8];
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public int getSeqno() {
		return seqno;
	}

	public void setSeqno(int seqno) {
		this.seqno = seqno;
	}

	public int getSaddr() {
		return saddr;
	}

	public void setSaddr(int saddr) {
		this.saddr = saddr;
	}

	public byte[] getDev() {
		return dev;
	}

	public void setDev(byte[] dev) {
		this.dev = dev;
	}

	public byte[] getMac() {
		return this.mac;
	}

	public void setMac(byte[] mac) {
		this.mac = mac;
	}

	public int getEventN() {
		return eventN;
	}

	public void setEventN(int eventN) {
		this.eventN = eventN;
	}

	public static String strDev(byte[] dev) {
        StringBuilder sb = new StringBuilder();
        for (byte b : dev) {
            sb.append(String.format("%02X", b));
        }
        return sb.toString();
    }
	
	public static byte[] toDev(String str) {
		if (str.indexOf(' ') != -1)
			str = str.replaceAll(" ", "");
		if (str.length() != 12)
			throw new RuntimeException("invalid devno string '" + str + "'");

		byte[] dev = new byte[6];
		for (int i = 0; i < 6; i++) {
			dev[i] = (byte)Integer.parseInt(str.substring(2*i, 2*i+2), 16);
		}
		return dev;
	}

	@Override
	public boolean equals(Object obj) {
		if (!(obj instanceof Mote))
			return false;
		Mote m = (Mote) obj;
		if (saddr != m.getSaddr())
			return false;
		if (seqno != m.getSeqno())
			return false;
		if (!name.equals(m.getName()))
			return false;
		if (dev.length != m.getDev().length)
			return false;
		for (int i = 0; i < dev.length; i++) {
			if (dev[i] != m.getDev()[i])
				return false;
		}
		return true;
	}

	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		for (byte b : dev)
			sb.append(String.format("%02X", b));
		sb.append(" - ").append(name);
		return sb.toString();
	}

}
