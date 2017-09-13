package com.cadre.wvds.wvdsota;

public class Subnet {

	public int id;
	public String city;
	public String district;
	public String street;
	public int rfchan;
	public int rfpower;
	public int vdVer;
	public String vdFile;
	public int vdSize;
	public int rpVer;
	public String rpFile;
	public int rpSize;
	public int apVer;
	public String apFile;
	public int apSize;

	public Subnet() { }

	public int getFwVer(String type) {
		if ("VD".equals(type)) return vdVer;
		if ("RP".equals(type)) return rpVer;
		if ("AP".equals(type)) return apVer;
		return 0;
	}

	public void setFwVer(String type, int ver) {
		if ("VD".equals(type)) vdVer = ver;
		if ("RP".equals(type)) rpVer = ver;
		if ("AP".equals(type)) apVer = ver;
	}

	public String getFwFile(String type) {
		if ("VD".equals(type)) return vdFile;
		if ("RP".equals(type)) return rpFile;
		if ("AP".equals(type)) return apFile;
		return "N/A";
	}

	public void setFwFile(String type, String file) {
		if ("VD".equals(type)) vdFile = file;
		if ("RP".equals(type)) rpFile = file;
		if ("AP".equals(type)) apFile = file;
	}

	public int getFwSize(String type) {
		if ("VD".equals(type)) return vdSize;
		if ("RP".equals(type)) return rpSize;
		if ("AP".equals(type)) return apSize;
		return 0;
	}

	public void setFwSize(String type, int size) {
		if ("VD".equals(type)) vdSize = size;
		if ("RP".equals(type)) rpSize = size;
		if ("AP".equals(type)) apSize = size;
	}

	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append(city)
			.append(" - ").append(district)
			.append(" - ").append(street);
		return sb.toString();
	}
}
