package net.skaldchen.commons.comm;

import java.util.ArrayList;
import java.util.List;

public abstract class PacketFilter {

    public static final int MATCHED = 1;
    public static final int NOMATCH = 2;
    public static final int WAIT    = 3;

    private String name = "NA";

    protected List<byte[]> packets;

    public PacketFilter(String name) {
        this.name = name;
        this.packets = new ArrayList<byte[]>();
    }

    public String getName() {
        return this.name;
    }

    protected void addPacket(byte[] packet) {
        packets.add(packet);
    }

    public byte[] getPacket() {
        if (packets.size() > 0) {
            byte[] packet = packets.get(0);
            packets.remove(0);
            return packet;
        }
        return null;
    }

    public abstract int filter(byte[] data);

    public abstract void reset();

}
