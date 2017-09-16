package net.skaldchen.commons.comm;

import net.skaldchen.commons.utils.BytesUtil;

public class HDLCFilter extends PacketFilter {

    private byte[] packet = null;

    private final byte FLAG_BYTE = 0x7E;
    private final byte ESC_BYTE  = 0x7D;

    private boolean insyn = false;
    private boolean inesc = false;
    private byte[]  raw   = new byte[128];
    private int     raw_i = 0;
    private byte[]  dec   = new byte[128];
    private int     dec_i = 0;

    public HDLCFilter() {
        super("HDLC");
    }

    @Override
    public int filter(byte[] data) {
        for (int i = 0; i < data.length; i++) {
            byte b = data[i];
            if (!insyn) {
                if (b == FLAG_BYTE) {
                    insyn = true;
                    dec_i = 0;
                    dec[dec_i++] = b;
                    raw_i = 0;
                    raw[raw_i++] = b;
                }
            }
            else {
                raw[raw_i++] = b;
                if (inesc) {
                    if (b == FLAG_BYTE) {
                        insyn = false;
                        dec_i = 0;
                        raw_i = 0;
                        continue; // invalid sync
                    }
                    dec[dec_i++] = (byte) (b ^ 0x20);
                    inesc = false;
                }
                else if (b == ESC_BYTE) {
                    inesc = true;
                }
                else if (b == FLAG_BYTE) {
                    if (dec_i == 1) {
                        dec_i = 0;
                        dec[dec_i++] = b;
                        raw_i = 0;
                        raw[raw_i++] = b;
                        continue; // start next packet
                    }
                    else if (dec_i < 4) {
                        insyn = false;
                        dec_i = 0;
                        raw_i = 0;
                        continue; // packet too small
                    }
                    dec[dec_i++] = b;

                    //packet = BytesUtil.getBytes(dec, 4, dec_i - 7);
                    //packet = BytesUtil.getBytes(raw, 0, raw_i);
                    packet = BytesUtil.getBytes(dec, 1, dec_i-2);
                    addPacket(packet);

                    return PacketFilter.MATCHED;
                }
                else {
                    dec[dec_i++] = b;
                }
            }
        }
        return (insyn ? PacketFilter.WAIT : PacketFilter.NOMATCH);
    }

    @Override
    public void reset() {
        dec_i = 0;
        raw_i = 0;
        insyn = false;
    }

}
