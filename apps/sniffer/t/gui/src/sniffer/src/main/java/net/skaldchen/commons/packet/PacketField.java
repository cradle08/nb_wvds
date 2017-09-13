package net.skaldchen.commons.packet;

public class PacketField {

    public String name;
    public String type;
    public String endian;
    public byte[] value;
    public String format;

    public PacketField() { }

    public int toInt() {
        int val = 0;

        if (type.equals("uint8")) {
            val = (value[0] & 0xff);
        }
        else if (type.equals("int8")) {
            val = value[0];
        }
        else if (type.equals("uint16")) {
            if (endian.equals("little"))
                val = (value[0] & 0xff) + ((value[1] & 0xff) << 8);
            else
                val = ((value[0] & 0xff) << 8) + (value[1] & 0xff);
        }
        else if (type.equals("int16")) {
            if (endian.equals("little"))
                val = (value[0] & 0xff) + ((value[1] & 0xff) << 8);
            else
                val = ((value[0] & 0xff) << 8) + (value[1] & 0xff);
            if ((val & 0x8000) != 0)
                val = -(0xffff - val + 1);
        }
        else if (type.equals("uint32")) {
            if (endian.equals("little"))
                val = (value[0] & 0xff) + ((value[1] & 0xff) << 8)
                    + ((value[2] & 0xff) << 16) + ((value[3] & 0xff) << 24);
            else
                val = ((value[0] & 0xff) << 24) + ((value[1] & 0xff) << 16)
                    + ((value[2] & 0xff) << 8) + (value[3] & 0xff);
        }
        else {
            throw new RuntimeException("not support type " + type);
        }

        return val;
    }

    public String toStr() {
        StringBuilder sb = new StringBuilder();

        if (type.equals("uint8") || type.equals("int8") ||
                type.equals("uint16") || type.equals("int16") ||
                type.equals("uint32") || type.equals("int32")) {
            sb.append((format == null ? toInt() : String.format(format, toInt()) ));
        }
        else if (type.startsWith("bits")) {
            sb.append(value[0]);
        }
        else if (type.startsWith("char")) {
            for (byte b : value) {
                if (b == 0)
                    break;
                sb.append((char)b);
            }
        }
        else if (type.startsWith("byte")) {
            for (byte b : value) {
                sb.append(String.format((format == null ? "%02X " : format), b));
            }
        }
        else {
            throw new RuntimeException("not support type " + type);
        }

        return sb.toString();
    }
}
