package net.skaldchen.commons.comm;

import net.skaldchen.commons.utils.BytesUtil;

public class CRLFFilter extends PacketFilter {

	private byte[] buf = new byte[256];
	private int idx = 0;
	private int s = 0;
	
	public CRLFFilter() {
		super("CRLF");
	}

	@Override
	public int filter(byte[] data) {
		for (int i = 0; i < data.length; i++) {
			if (data[i] == 0x0A) {
				buf[idx++] = data[i];
				s = 2;
				byte[] pkt = BytesUtil.getBytes(buf, 0, idx);
				addPacket(pkt);
				if (i < data.length-1) reset();
			} else {
				buf[idx++] = data[i];
				s = 1;
			}
		}
		
		if (s == 2) {
			return MATCHED;
		} else if (s == 1) {
			return WAIT;
		}
		return NOMATCH;
	}

	@Override
	public void reset() {
		idx = 0;
		s = 0;
	}

}
