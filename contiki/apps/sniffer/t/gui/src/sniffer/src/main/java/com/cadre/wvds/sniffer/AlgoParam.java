package com.cadre.wvds.sniffer;

public class AlgoParam {

	public int normalT;
	public int flunctT;
	public int bigOccThr;
	public int midOccThr;
	public int litOccThr;
	public int unOccThr;
	public int baseLineX;
	public int baseLineY;
	public int baseLineZ;
	public int sensorGain;

	private final int LEN = 15;

	public AlgoParam() {

	}

	public byte[] toBytes() {
		byte[] out = new byte[LEN];
		int ofs = 0;
		putBEInt16(normalT, out, ofs); ofs += 2;
		putBEInt16(flunctT, out, ofs); ofs += 2;
		putInt8(bigOccThr, out, ofs); ofs += 1;
		putInt8(midOccThr, out, ofs); ofs += 1;
		putInt8(litOccThr, out, ofs); ofs += 1;
		putInt8(unOccThr, out, ofs); ofs += 1;
		putBEInt16(baseLineX, out, ofs); ofs += 2;
		putBEInt16(baseLineY, out, ofs); ofs += 2;
		putBEInt16(baseLineZ, out, ofs); ofs += 2;
		putInt8(sensorGain, out, ofs); ofs += 1;
		return out;
	}

	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("{");
		sb.append("normalT=").append(normalT).append(",");
		sb.append("flunctT=").append(flunctT).append(",");
		sb.append("bigOccThr=").append(bigOccThr).append(",");
		sb.append("midOccThr=").append(midOccThr).append(",");
		sb.append("litOccThr=").append(litOccThr).append(",");
		sb.append("unOccThr=").append(unOccThr).append(",");
		sb.append("baseLineX=").append(baseLineX).append(",");
		sb.append("baseLineY=").append(baseLineY).append(",");
		sb.append("baseLineZ=").append(baseLineZ).append(",");
		sb.append("sensorGain=").append(sensorGain);
		sb.append("}");
		return sb.toString();
	}

	public static AlgoParam create(byte[] data, int ofs) {
		AlgoParam algo = new AlgoParam();
		algo.normalT = getBEInt16(data, ofs); ofs += 2;
		algo.flunctT = getBEInt16(data, ofs); ofs += 2;
		algo.bigOccThr = getInt8(data, ofs); ofs += 1;
		algo.midOccThr = getInt8(data, ofs); ofs += 1;
		algo.litOccThr = getInt8(data, ofs); ofs += 1;
		algo.unOccThr = getInt8(data, ofs); ofs += 1;
		algo.baseLineX = getBEInt16(data, ofs); ofs += 2;
		algo.baseLineY = getBEInt16(data, ofs); ofs += 2;
		algo.baseLineZ = getBEInt16(data, ofs); ofs += 2;
		algo.sensorGain = getInt8(data, ofs); ofs += 1;
		return algo;
	}

	private static int getInt8(byte[] data, int ofs) {
		int val = (data[ofs] & 0xFF);
		if ((val & 0x80) > 0)
			val -= 256;
		return val;
	}

	private static void putInt8(int val, byte[] data, int ofs) {
		data[ofs] = (byte)(val > 0 ? val : (256+val));
	}

	private static int getBEInt16(byte[] data, int ofs) {
		int val = ((data[ofs] & 0xFF) << 8) + (data[ofs+1] & 0xFF);
		if ((val & 0x8000) > 0)
			val -= 65536;
		return val;
	}

	private static void putBEInt16(int val, byte[] data, int ofs) {
		data[ofs + 0] = (byte) ((val >>  8) & 0xff);
		data[ofs + 1] = (byte) ((val >>  0) & 0xff);
	}

}
