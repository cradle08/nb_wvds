package net.skaldchen.commons.serial;

import net.skaldchen.commons.utils.BytesUtil;

public class HDLC {

    public static final byte PROTO_ACK          = 0x43;
    public static final byte PROTO_PACKET_ACK   = 0x44;
    public static final byte PROTO_PACKET_NOACK = 0x45;

    public static final byte FLAG_BYTE = 0x7E;
    public static final byte ESC_BYTE  = 0x7D;

    public static final int MAXLEN = 1024;

    public static byte[] encode(byte[] pkt, int proto, int seqno, int type) {
        byte[] buf = new byte[MAXLEN];
        int ccrc = 0x0000;
        byte b = 0;
        int inlen = pkt.length;
        int idx = 0;

        buf[idx++] = FLAG_BYTE;
        for (int j = 0; j < 3+inlen+2; j++) {
            if (proto == PROTO_PACKET_ACK || proto == PROTO_PACKET_NOACK) {
                if (j < 1) b = (byte) proto;
                else if (j < 2) b = (byte) seqno;
                else if (j < 3) b = (byte) type;
                else if (j < 3+inlen) b = pkt[j-3];
                else if (j < 3+inlen+1) b = (byte) (ccrc & 0xFF);
                else if (j < 3+inlen+2) b = (byte) (ccrc >> 8);

                if (j < 3+inlen)
                    ccrc = crc_byte(ccrc, b);
            }
            else if (proto == PROTO_ACK) {
                if (j < 1) b = (byte) proto;
                else if (j < 2) b = pkt[j-1];
                else if (j < 3) b = (byte) (ccrc & 0xFF);
                else if (j < 4) b = (byte) (ccrc >> 8);
                else continue;

                if (j < 2)
                    ccrc = crc_byte(ccrc, b);
            }

            if (b == FLAG_BYTE || b == ESC_BYTE) {
                buf[idx++] = ESC_BYTE;
                buf[idx++] = (byte) (b ^ 0x20);
            } else {
                buf[idx++] = b;
            }
        }
        buf[idx++] = FLAG_BYTE;

        return BytesUtil.getBytes(buf, 0, idx);
    }

    public static byte[] decode(byte[] rbuf, int ofs, int rlen) {
        byte[] rxbuf = new byte[rlen];
        int rxidx = 0;
        int rxlen = 0;

        boolean insyn = false;
        boolean inesc = false;
        final byte HDLC_FLAG_BYTE = 0x7E;
        final byte HDLC_ESC_BYTE  = 0x7D;

        for (int i = ofs; i < ofs + rlen; i++) {
            byte b = rbuf[i];
            if (!insyn) {
                if (b == HDLC_FLAG_BYTE) {
                    insyn = true;
                    rxidx = 0;
                    rxbuf[rxidx++] = b;
                }
            }
            else {
                if (inesc) {
                    if (b == HDLC_FLAG_BYTE) {
                        insyn = false;
                        rxidx = 0;
                        continue; // invalid sync
                    }
                    rxbuf[rxidx++] = (byte) (b ^ 0x20);
                    inesc = false;
                }
                else if (b == HDLC_ESC_BYTE) {
                    inesc = true;
                }
                else if (b == HDLC_FLAG_BYTE) {
                    if (rxidx == 1) {
                        rxidx = 0;
                        rxbuf[rxidx++] = b;
                        continue; // start next packet
                    }
                    else if (rxidx < 4) {
                        insyn = false;
                        rxidx = 0;
                        continue; // packet too small
                    }

                    rxbuf[rxidx++] = b;
                    rxlen = rxidx;
                    rxidx = 0;
                    insyn = false;

                    byte[] pkt = BytesUtil.getBytes(rxbuf, 4, rxlen - 7);
                    return pkt;
                }
                else {
                    rxbuf[rxidx++] = b;
                }
            }
        }
        return null;
    }

    private static int crc_byte(int crc, byte b) {
        int i;

        crc = crc ^ (b << 8);
        i = 8;
        do {
            if ((crc & 0x8000) > 0)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = (crc << 1);
        } while (--i != 0);

        return (crc & 0xffff);
    }

}
