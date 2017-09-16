package com.cadre.wvds.sniffer;

import java.io.IOException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.serial.CommAdapter;
import net.skaldchen.commons.serial.PacketListener;
import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.Config;

public class Network implements PacketListener {

    private static final Logger log = LoggerFactory.getLogger(Network.class);

    protected CommAdapter comm;
    private byte cmdSeq = 1;

    public Network(CommAdapter comm) {
        this.comm = comm;
    }

    public void getFirmwareVer() {
        sendCommand(Const.SNIFFER_CMD_GETVER, new byte[]{}, false);
    }

    public void getChannel() {
        sendCommand(Const.SNIFFER_CMD_GETCHAN, new byte[]{}, false);
    }

    public void getTXPower() {
        sendCommand(Const.SNIFFER_CMD_GETPWR, new byte[]{}, false);
    }

    public void sendCommand(int op, byte[] arg, boolean ack) {
        if (comm == null) {
            log.warn("no comm to send");
            return;
        }

        byte[] packet = new byte[5 + arg.length];
        int i = 0;
        packet[i++] = Const.SNIFFER_CMD;
        packet[i++] = getCmdSeq();
        packet[i++] = (byte) (ack ? 0x01 : 0x00);
        packet[i++] = (byte) op;
        packet[i++] = (byte) arg.length;
        for (int j = 0; j < arg.length; j++)
            packet[i++] = arg[j];

        try {
            log.debug(String.format("send: (%d) %s", packet.length, BytesUtil.strBytes(packet)));
            comm.send(packet);
        } catch (IOException e) {
            log.warn("fail send command " + op, e);
        }
    }

    private byte getCmdSeq() {
        return ++cmdSeq;
    }

    @Override
    public void packetReceived(byte[] pkt) {
        log.debug(String.format("rcvd: (%d) %s", pkt.length, BytesUtil.strBytes(pkt)));
        if ("hdlc".equals(Config.get("serial-proto"))) {
            if (pkt[0] == Const.SNIFFER_ACK) {
                if (pkt[1] == cmdSeq) {
                    Main.gui.showOutput("²Ù×÷³É¹¦");
                } else {
                    log.warn(String.format("ack %d mismatch cmd %d", (pkt[1] & 0xff), (cmdSeq & 0xff)));
                }
            }
            else if (pkt[0] == Const.SNIFFER_DATA) {
                if (pkt[2] == Const.SNIFFER_DATA_CHAN) {
                    int chan = BytesUtil.getUInt8(pkt, 4);
                    Config.set("radio-channel", String.valueOf(chan), true);
                }
            }
            else if (pkt[0] == Const.SNIFFER_CMD) {
                // nothing need to do
            }
            else if (pkt[0] == Const.SNIFFER_MSG) {
                // nothing need to do
            }
            else {
                log.warn(String.format("unknown message, type 0x%02X", pkt[0]));
            }
        }
    }
}
