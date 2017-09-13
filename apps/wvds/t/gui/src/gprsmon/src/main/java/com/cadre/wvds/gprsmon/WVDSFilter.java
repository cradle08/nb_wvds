package com.cadre.wvds.gprsmon;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.comm.PacketFilter;
import net.skaldchen.commons.util.BytesUtil;

public class WVDSFilter extends PacketFilter {

    private static final Logger log = LoggerFactory.getLogger(WVDSFilter.class);

    private final int S_START = 0;
    private final int S_BEG   = 1;
    private final int S_LEN   = 2;
    private final int S_DATA  = 3;
    private final int S_END   = 4;
    private int s = S_START;

    private byte[]  rxbuf = new byte[256];
    private int     rxidx = 0;
    private int     rxlen = 0;

    public WVDSFilter() {
        super("WVDS");
    }

    @Override
    public int filter(byte[] data) {
        try {
            log.debug(String.format("handle: (%d) %s", data.length, BytesUtil.strBytes(data)));
            log.debug(String.format("  s %d, rxidx %d, rxlen %d", s, rxidx, rxlen));
            for (int i = 0; i < data.length; i++) {
                byte b = data[i];

                if (b == (byte)0xAA) {
                    log.debug(String.format("new start from %d", i));
                    s = S_BEG;
                    rxidx = 0;
                    rxbuf[rxidx++] = b;
                    continue;
                }

                if (s == S_BEG) {
                    s = S_LEN;
                    rxbuf[rxidx++] = b;
                    rxlen = (b & 0xff);
                }
                else if (s == S_LEN) {
                    s = S_DATA;
                    rxbuf[rxidx++] = b;
                }
                else if (s == S_DATA) {
                    rxbuf[rxidx++] = b;
                    if (rxidx >= rxlen + 5) {
                        if (b == (byte)0xFF) {
                            s = S_END;

                            byte[] pkt = BytesUtil.getBytes(rxbuf, 0, rxidx);
                            log.debug(String.format("got: (%d) %s", pkt.length, BytesUtil.strBytes(pkt)));
                            addPacket(pkt);
                            if (i < data.length-1) reset();
                        }
                        else {
                            log.warn(String.format("not valid packet: (%d) %s", rxidx, BytesUtil.strBytes(rxbuf, 0, rxidx)));
                            reset();
                        }
                    }
                }
            }
            log.debug(String.format("  s %d, rxidx %d, rxlen %d", s, rxidx, rxlen));
        } catch(Exception e) {
            log.warn("fail filter", e);
            reset();
        }

        if (s == S_END) {
            reset(); // prepare for next packet
            return PacketFilter.MATCHED;
        }
        else if (s == S_START) {
            return PacketFilter.NOMATCH;
        }
        else {
            return PacketFilter.WAIT;
        }
    }

    @Override
    public void reset() {
        log.debug("reset");
        s = S_START;
        rxidx = 0;
        rxlen = 0;
    }

}
