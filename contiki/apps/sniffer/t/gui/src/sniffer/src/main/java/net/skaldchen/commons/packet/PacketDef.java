package net.skaldchen.commons.packet;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Created with IntelliJ IDEA.
 * User: Administrator
 * Date: 13-11-20
 * Time: 上午10:08
 * To change this template use File | Settings | File Templates.
 */
public class PacketDef {

    private String name;
    private List<Field> fields = new ArrayList<Field>();

    public PacketDef () { }

    public String getName() { return this.name; }
    public void setName(String name) { this.name = name; };

    public void addField(String name, String type, String endian, int offset, int length, byte[] value, String format, boolean hide) {
        Field f = new Field();
        f.name = name;
        f.type = type;
        f.endian = endian;
        f.offset = offset;
        f.length = length;
        f.value = value;
        f.format = format;
        f.hide = hide;
        fields.add(f);
    }

    public PacketDef.Field getField(String name) {
        for (Field f : fields) {
            if (name.equals(f.name))
                return f;
        }
        return null;
    }

    public boolean valid() {
        for (Field f : fields) {
            if (f.value != null)
                return true;
        }
        System.err.println("packet definition not valid\n" + this);
        return false;
    }

    public boolean match(byte[] packet) {
        for (Field f : fields) {
            if (packet.length < f.offset + f.length)
                return false;
            if (f.value != null) {
                byte[] val = new byte[f.length];
                System.arraycopy(packet, f.offset, val, 0, f.length);
                if (f.type.startsWith("bits")) {
                    String[] subs = f.type.substring(5, 8).split(":");
                    int h = Integer.parseInt(subs[0]);
                    int l = Integer.parseInt(subs[1]);
                    int mask = 0;
                    for (int i = l; i <= h; i++) {
                        mask |= (1 << i);
                    }
                    if ((val[0] & mask) != f.value[0] )
                        return false;
                } else {
                    if (!Arrays.equals(val, f.value))
                        return false;
                }
            }
        }
        return true;
    }

    public Packet createFrom(byte[] packet) {
        try {
            Packet pkt = new Packet();
            pkt.setName(name);
            pkt.setBytes(packet);
            for (Field f : fields) {
                byte[] val = new byte[f.length];
                if (f.type.startsWith("bits")) {
                    String[] subs = f.type.substring(5, 8).split(":");
                    int h = Integer.parseInt(subs[0]);
                    int l = Integer.parseInt(subs[1]);
                    int mask = 0;
                    for (int i = l; i <= h; i++) {
                        mask |= (1 << i);
                    }
                    val[0] = (byte) ((packet[f.offset] & mask) >> l);
                } else {
                    System.arraycopy(packet, f.offset, val, 0, f.length);
                }
                pkt.addField(f.name, f.type, f.endian, val, f.format);
            }
            pkt.setDef(this);
            return pkt;
        } catch(Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(name);
        for (Field f : fields) {
            sb.append("\n  ").append(f.name).append(", ").append(f.type).append(", ").append(f.length);
        }
        sb.append("\n");
        return sb.toString();
    }

    // <field offset="0" name="to" type="uint16" endian="little"/>
    public class Field {
        public String name;
        public String type;
        public String endian;
        public int offset;
        public int length;
        public byte[] value;
        public boolean hide;
        public String format;

        public Field() { }
    }
}
