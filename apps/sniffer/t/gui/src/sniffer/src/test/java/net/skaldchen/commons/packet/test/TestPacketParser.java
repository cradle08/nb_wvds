package net.skaldchen.commons.packet.test;

import net.skaldchen.commons.packet.Packet;
import net.skaldchen.commons.packet.PacketParser;

public class TestPacketParser {

    public static void main(String[] args) {
        PacketParser parser = new PacketParser("packet.xml");

        String str = null;
        //str = "00 00 E6 03 91 00 1B FE 49 FF FC FE 11";
        str = "00 00 01 00 38 00 01 00 44 00 02 A9 00 00";
        String[] subs = str.split(" ");
        byte[] pkt = new byte[subs.length];
        for (int i = 0; i < pkt.length; i++)
            pkt[i] = (byte)Integer.parseInt(subs[i], 16);

        Packet p = parser.parse(pkt);
        if (p != null) {
            System.out.println(p.getName() + ": " + p.toDescription(", "));
            System.out.println(p.toString());
        } else {
            System.err.println("failed parse");
        }
    }

}
