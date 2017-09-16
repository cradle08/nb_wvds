package net.skaldchen.commons.packet;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Created with IntelliJ IDEA.
 * User: Administrator
 * Date: 13-11-20
 * Time: 上午11:29
 * To change this template use File | Settings | File Templates.
 */
public class Packet {

    private String name;
    private List<PacketField> fields = new ArrayList<PacketField>();
    private byte[] bytes;
    private PacketDef def = null;

    public Packet() { }

    public String getName() { return this.name; }
    public void setName(String name) { this.name = name; };

    public byte[] getBytes() { return this.bytes; }
    public void setBytes(byte[] bytes) { this.bytes = bytes; }

    public PacketDef getDef() { return this.def; }
    public void setDef(PacketDef def) { this.def = def; }

    public void addField(String name, String type, String endian, byte[] value, String format) {
        PacketField f = new PacketField();
        f.name = name;
        f.type = type;
        f.endian = endian;
        f.value = value;
        f.format = format;
        fields.add(f);
    }

    public PacketField getField(String name) {
        for (PacketField f : fields) {
            if (name.equals(f.name))
                return f;
        }
        return null;
    }

    public String toDescription(String sep) {
        StringBuilder sb = new StringBuilder();
        for (PacketField f : fields) {
            if (def.getField(f.name).hide)
                continue;
            if (sb.length() > 0)
                sb.append(sep);
            sb.append(f.name).append("=").append(f.toStr());
        }
        return sb.toString();
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (PacketField f : fields) {
            sb.append(f.name).append(" (").append(f.type).append(") = ")
                    .append(strBytes(f.value)).append("\n");
        }
        return sb.toString();
    }

    public static String strBytes(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes)
            sb.append(String.format("%02X ", b));
        return sb.substring(0, sb.length()-1);
    }

}
