package com.cadre.wvds.gprsmon;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.comm.PacketFilter;
import net.skaldchen.commons.util.BytesUtil;

public class M26Filter extends PacketFilter {

    private static final Logger log = LoggerFactory.getLogger(M26Filter.class);

    private final int M26_RX_START    = 0;
    private final int M26_RX_BEG_CR   = 1;
    private final int M26_RX_BEG_LF   = 2;
    private final int M26_RX_PROMPT   = 3;
    private final int M26_RX_DATA     = 4;
    private final int M26_RX_END_CR   = 5;
    private final int M26_RX_END_LF   = 6;
    private final int M26_RX_BEG_A    = 7;
    private final int M26_RX_BEG_T    = 8;
    private final int M26_RX_CMD      = 9;
    private final int M26_RX_END_CMD  = 10;
    private final int M26_RX_IPD_I    = 11;
    private final int M26_RX_IPD_P    = 12;
    private final int M26_RX_IPD_D    = 13;
    private final int M26_RX_IPD_LEN  = 14;
    private final int M26_RX_IPD_DATA = 15;
    private final int M26_RX_AA       = 16;
    private final int M26_RX_LEN      = 17;
    private final int M26_RX_DAT      = 18;
    private final int M26_RX_FF       = 19;
    private final int M26_RX_DONE     = 20;
    private int m26_rx = 0;

    private byte[] m26_pkt = new byte[1024];
    private int m26_pkti = 0;
    private int m26_ipdn = 0;
    private int m26_ipdi = 0;
    private int m26_rxlen = 0;

    public M26Filter() {
        super("M26");
    }

    private void m26_frame_rcvd(byte[] frame, int len) {
        byte[] pkt = new byte[len];
        System.arraycopy(frame, 0, pkt, 0, len);
        log.debug(String.format("got: (%d) %s", pkt.length, BytesUtil.strBytes(pkt)));
        addPacket(pkt);
    }

    @Override
    public int filter(byte[] data) {
        try {
            log.debug(String.format("handle: (%d) %s", data.length, BytesUtil.strBytes(data)));
            log.debug(String.format("  s %d, pkti %d, ipdi %d, ipdn %d", m26_rx, m26_pkti, m26_ipdi, m26_ipdn));
            for (int i = 0; i < data.length; i++) {
                byte b = data[i];

                if (m26_rx == M26_RX_START) {
                    if (b == 0x0D) {
                        m26_rx = M26_RX_BEG_CR;
                        m26_pkt[m26_pkti++] = b;
                    }
                    else if (b == 'A') {
                        m26_rx = M26_RX_BEG_A;
                        m26_pkt[m26_pkti++] = b;
                    }
                    else if (b == 'I') {
                        m26_rx = M26_RX_IPD_I;
                        m26_pkt[m26_pkti++] = b;
                    }
                    else if (b == (byte)0xAA) {
                        m26_rx = M26_RX_AA;
                        m26_pkt[m26_pkti++] = b;
                    }
                }
                else if (m26_rx == M26_RX_AA) {
                    m26_rx = M26_RX_LEN;
                    m26_pkt[m26_pkti++] = b;
                    m26_rxlen = (b & 0xff) + 5;
                }
                else if (m26_rx == M26_RX_LEN) {
                    m26_pkt[m26_pkti++] = b;
                    if (m26_pkti == m26_rxlen) {
                        m26_rx = M26_RX_DAT;
                        if (b == (byte)0xFF) {
                            m26_rx = M26_RX_DONE;
                            m26_frame_rcvd(m26_pkt, m26_pkti); // rcvd echo WVDS msg
                            if (i < data.length-1) reset();
                        } else {
                            reset();
                        }
                    }
                }
                else if (m26_rx == M26_RX_BEG_A) {
                    if (b == 'T') {
                        m26_rx = M26_RX_BEG_T;
                        m26_pkt[m26_pkti++] = b;
                    }
                    else if (b == 'A') {
                        // maybe new start
                    }
                    else {
                        m26_rx = M26_RX_START;
                        m26_pkti = 0;
                    }
                }
                else if (m26_rx == M26_RX_BEG_T) {
                    if (b == 0x0D) {
                        m26_rx = M26_RX_END_CMD;
                        m26_pkt[m26_pkti++] = b;
                        m26_rx = M26_RX_DONE;
                        m26_frame_rcvd(m26_pkt, m26_pkti); // rcvd AT\r
                        if (i < data.length-1) reset();
                    }
                    else {
                        m26_rx = M26_RX_CMD;
                        m26_pkt[m26_pkti++] = b;
                    }
                }
                else if (m26_rx == M26_RX_CMD) {
                    if (b == 0x0D) {
                        m26_rx = M26_RX_END_CMD;
                        m26_pkt[m26_pkti++] = b;
                        m26_rx = M26_RX_DONE;
                        m26_frame_rcvd(m26_pkt, m26_pkti); // rcvd AT+
                        if (i < data.length-1) reset();
                    }
                    else {
                        m26_pkt[m26_pkti++] = b;
                    }
                }
                else if (m26_rx == M26_RX_BEG_CR) {
                    if (b == 0x0A) {
                        m26_rx = M26_RX_BEG_LF;
                        m26_pkt[m26_pkti++] = b;
                    }
                    else if (b == 0x0D) {
                        // maybe new start
                    }
                    else {
                        m26_rx = M26_RX_START;
                        m26_pkti = 0;
                    }
                }
                else if (m26_rx == M26_RX_BEG_LF) {
                    if (b == 0x0D) {
                        m26_rx = M26_RX_BEG_CR;
                        m26_pkti = 0;
                        m26_pkt[m26_pkti++] = b;
                    }
                    else if (b == 0x3E) { // >
                        m26_rx = M26_RX_PROMPT;
                        m26_pkt[m26_pkti++] = b;
                    }
                    else {
                        m26_rx = M26_RX_DATA;
                        m26_pkt[m26_pkti++] = b;
                    }
                }
                else if (m26_rx == M26_RX_PROMPT) {
                    if (b == 0x20) { // space
                        m26_pkt[m26_pkti++] = b;
                        m26_rx = M26_RX_DONE;
                        m26_frame_rcvd(m26_pkt, m26_pkti); // rcvd \r\n>
                        if (i < data.length-1) reset();
                    } else {
                        m26_rx = M26_RX_DATA;
                        m26_pkt[m26_pkti++] = b;
                    }
                }
                else if (m26_rx == M26_RX_DATA) {
                    if (b == 0x0D) {
                        m26_rx = M26_RX_END_CR;
                        m26_pkt[m26_pkti++] = b;
                    } else {
                        m26_pkt[m26_pkti++] = b;
                    }
                }
                else if (m26_rx == M26_RX_END_CR) {
                    if (b == 0x0A) {
                        m26_rx = M26_RX_END_LF;
                        m26_pkt[m26_pkti++] = b;
                        m26_rx = M26_RX_DONE;
                        m26_frame_rcvd(m26_pkt, m26_pkti); // rcvd \r\n...\r\n
                        if (i < data.length-1) reset();
                    } else {
                        m26_rx = M26_RX_START;
                        m26_pkti = 0;
                    }
                }
                else if (m26_rx == M26_RX_IPD_I) {
                    if (b == 'P') {
                        m26_rx = M26_RX_IPD_P;
                        m26_pkt[m26_pkti++] = b;
                    } else if (b == 'I') {
                        // may be new start
                    } else {
                        m26_rx = M26_RX_START;
                        m26_pkti = 0;
                    }
                }
                else if (m26_rx == M26_RX_IPD_P) {
                    if (b == 'D') {
                        m26_rx = M26_RX_IPD_D;
                        m26_pkt[m26_pkti++] = b;
                    } else {
                        m26_rx = M26_RX_START;
                        m26_pkti = 0;
                    }
                }
                else if (m26_rx == M26_RX_IPD_D) {
                    if ('0' <= b && b <= '9') {
                        m26_rx = M26_RX_IPD_LEN;
                        m26_pkt[m26_pkti++] = b;
                        m26_ipdn = b - '0';
                    } else {
                        m26_rx = M26_RX_START;
                        m26_pkti = 0;
                    }
                }
                else if (m26_rx == M26_RX_IPD_LEN) {
                    if (b == ':') {
                        m26_pkt[m26_pkti++] = b;
                        m26_rx = M26_RX_IPD_DATA;
                        m26_ipdi = 0;
                    } else if ('0' <= b && b <= '9') {
                        m26_pkt[m26_pkti++] = b;
                        m26_ipdn = (m26_ipdn * 10) + (b - '0');
                    } else {
                        m26_rx = M26_RX_START;
                        m26_pkti = 0;
                    }
                }
                else if (m26_rx == M26_RX_IPD_DATA) {
                    m26_pkt[m26_pkti++] = b;
                    if (++m26_ipdi == m26_ipdn) {
                        m26_rx = M26_RX_DONE;
                        m26_frame_rcvd(m26_pkt, m26_pkti); // rcvd IPD...
                        if (i < data.length-1) reset();
                    }
                }
            }
            log.debug(String.format("  s %d, pkti %d, ipdi %d, ipdn %d", m26_rx, m26_pkti, m26_ipdi, m26_ipdn));
        } catch(Exception e) {
            log.warn("fail filter", e);
            reset();
        }

        if (m26_rx == M26_RX_DONE) {
            reset(); // prepare for next packet
            return MATCHED;
        }
        else if (m26_rx == M26_RX_START) {
            return NOMATCH;
        }
        return WAIT;
    }

    @Override
    public void reset() {
        log.debug("reset");
        m26_rx = M26_RX_START;
        m26_pkti = 0;
        m26_ipdi = 0;
        m26_ipdn = 0;
    }

}
