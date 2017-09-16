package net.skaldchen.commons.packet;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.dom4j.Document;
import org.dom4j.Element;

import net.skaldchen.commons.utils.Dom4JUtil;

/**
 * Created with IntelliJ IDEA.
 * User: Administrator
 * Date: 13-11-20
 * Time: 上午10:12
 * To change this template use File | Settings | File Templates.
 */
public class PacketParser {

    private String conf;
    private HashMap<String,Integer> typeMap;
    private List<PacketDef> packetDefs;

    public PacketParser(String conf) {
        typeMap = new HashMap<String, Integer>();
        typeMap.put("byte", Integer.valueOf(0));
        typeMap.put("uint8", Integer.valueOf(1));
        typeMap.put("uint16", Integer.valueOf(2));
        typeMap.put("uint32", Integer.valueOf(4));
        typeMap.put("int8", Integer.valueOf(1));
        typeMap.put("int16", Integer.valueOf(2));
        typeMap.put("int32", Integer.valueOf(4));
        typeMap.put("bits", Integer.valueOf(1));
        typeMap.put("char", Integer.valueOf(1));

        this.conf = conf;
        this.packetDefs = new ArrayList<PacketDef>();
        reload();
    }

    public void reload() {
        Document doc = Dom4JUtil.readXML(conf);
        ArrayList<Element> elems = Dom4JUtil.getElems(doc, "/packets/packet");
        packetDefs.clear();
        for (Element e : elems) {
            PacketDef p = new PacketDef();
            p.setName(e.attributeValue("name"));

            List<Element> fields = e.elements();
            for (Element f : fields) {
                String name = f.attributeValue("name");
                String type = f.attributeValue("type");
                String endian = f.attributeValue("endian");
                int offset = Integer.parseInt(f.attributeValue("offset"));
                int length = 0;
                byte[] value = null;
                String format = f.attributeValue("format");
                boolean hide = false;

                // decide length of value byte array
                if (f.attribute("length") == null) {
                    Integer len = typeMap.get(type);
                    if (type.matches("^.*\\[[0-9]:[0-9]\\]$")) { // bits[7:3]
                        len = Integer.valueOf(1);
                    }
                    else if (type.matches("^.*\\[[0-9]+\\]$")) { // char[20]
                        Pattern patt = Pattern.compile("\\[([0-9]+)\\]");
                        Matcher m = patt.matcher(type);
                        if (m.find())
                            len = Integer.valueOf(m.group(1));
                    }
                    if (len != null) {
                        length = len.intValue();
                    } else {
                        System.err.println("no length definition for type " + type);
                    }
                } else {
                    length = Integer.parseInt(f.attributeValue("length"));
                }
                // set value if exist
                if (f.attribute("value") != null) {
                    String svalue = f.attributeValue("value");
                    value = valueToBytes(svalue);
                }
                if (f.attribute("hide") != null) {
                    hide = "true".equals(f.attributeValue("hide"));
                }

                p.addField(name, type, endian, offset, length, value, format, hide);
            }
            // check whether rule valid, i.e. at least one field has value to match
            if (p.valid())
                packetDefs.add(p);
        }

        //for (PacketDef p : packetDefs)
        //    System.out.println(p.toString());
    }

    public Packet parse(byte[] packet) {
        for (PacketDef def : packetDefs) {
            if (def.match(packet)) {
                return def.createFrom(packet);
            }
        }
        return null;
    }

    public PacketDef getPacketDef(String name) {
        for (PacketDef def : packetDefs) {
            if (name.equals(def.getName()))
                return def;
        }
        return null;
    }

    public byte[] valueToBytes(String str) {
        String[] subs = str.split(" ");
        byte[] bytes = new byte[subs.length];
        for (int i = 0; i < bytes.length; i++) {
            if (subs[i].matches("^0x[0-9A-Fa-f]+$"))
                bytes[i] = (byte)Integer.parseInt(subs[i].substring(2), 16);
            else if (subs[i].matches("^[0-9]+$"))
                bytes[i] = (byte)Integer.parseInt(subs[i], 10);
            else
                throw new RuntimeException("invalid byte array string: " + str);
        }
        return bytes;
    }

}
