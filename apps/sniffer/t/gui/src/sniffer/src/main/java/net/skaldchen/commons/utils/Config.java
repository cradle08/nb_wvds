package net.skaldchen.commons.utils;

import java.awt.Color;
import java.awt.Font;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class Config {

    private static Logger log = LoggerFactory.getLogger(Config.class);

    private static String inifile = null;
    private static final String HINT = "set config file with Config.init() first";

    private static List<String> content = new ArrayList<String>();
    private static HashMap<String,String> entries = new HashMap<String,String>();
    private static HashMap<Integer,String> linemap = new HashMap<Integer,String>();
    private static HashMap<String,String> new_entries = new HashMap<String,String>();

    private static final String encoding = "cp936";

    public static void init(String fname) {
        inifile = fname;
        load();
    }

    public static boolean has(String key) {
        return entries.containsKey(key);
    }

    public static String get(String key) {
        if (inifile == null)
            throw new RuntimeException(Config.HINT);
        if (!entries.containsKey(key))
            log.warn("no entry " + key);
        return entries.get(key);
    }

    public static String get(String key, String dflt) {
        return (has(key) ? get(key) : dflt);
    }

    public static void set(String key, String val) {
        log.debug("set " + key + "=" + val);
        entries.put(key, val);
    }

    public static void set(String key, String val, boolean save) {
        log.debug("set " + key + "=" + val);
        if (inifile == null)
            throw new RuntimeException(Config.HINT);
        if (!entries.containsKey(key)) {
            new_entries.put(key, val);
        }
        entries.put(key, val);
        if (save)
            save();
    }


    public static boolean getBool(String key) {
        String val = get(key);
        if (!(val.equals("true") || val.equals("false")))
            log.warn("invalid bool value " + key);
        return "true".equals(val);
    }

    public static boolean getBool(String key, boolean dflt) {
        return (has(key) ? getBool(key) : dflt);
    }

    public static int getInt(String key) {
        int ret = -1;
        try {
            ret = Integer.parseInt(get(key));
        } catch (NumberFormatException e) {
            log.warn("invalid int value " + key);
        }
        return ret;
    }

    public static int getInt(String key, int dflt) {
        return (has(key) ? getInt(key) : dflt);
    }

    public static double getDouble(String key) {
        double ret = -1;
        try {
            ret = Double.parseDouble(get(key));
        } catch (NumberFormatException e) {
            log.warn("invalid double value " + key);
        }
        return ret;
    }

    public static double getDouble(String key, double dflt) {
        return (has(key) ? getDouble(key) : dflt);
    }

    public static Color getColor(String key) {
        Color c = Color.black;
        String val = get(key);
        if (val.matches("^#[0-9A-Fa-f]{6}$")) {
            int r = hexInt(val.substring(1, 3));
            int g = hexInt(val.substring(3, 5));
            int b = hexInt(val.substring(5, 7));
            c = new Color(r, g, b);
        }
        else if (val.matches("^#[0-9A-Fa-f]{8}$")) {
            int r = hexInt(val.substring(1, 3));
            int g = hexInt(val.substring(3, 5));
            int b = hexInt(val.substring(5, 7));
            int a = hexInt(val.substring(7, 9));
            c = new Color(r, g, b, a);
        }
        else {
            log.warn("invalid color value " + key + "=" + val);
        }
        return c;
    }

    public static Color getColor(String key, Color dflt) {
        return (has(key) ? getColor(key) : dflt);
    }

    public static Font getFont(String key) {
        String val = get(key);
        Font f = null;
        if (val.matches("^[A-Za-z ]*:[a-z]*:[0-9]*$")) {
            String[] subs = val.split(":");
            String name = subs[0];
            int face = Font.PLAIN;
            if ("plain".equalsIgnoreCase(subs[1]))
                face = Font.PLAIN;
            else if ("bold".equalsIgnoreCase(subs[1]))
                face = Font.BOLD;
            else if ("italic".equalsIgnoreCase(subs[1]))
                face = Font.ITALIC;
            int size = Integer.parseInt(subs[2]);
            return new Font(name, face, size);
        } else {
            System.err.println("Config: invalid font value " + val);
        }
        return f;
    }

    // TODO: add auto detection of file encoding
    private static void load() {
        File f = new File(inifile);
        if (inifile == null) {
            throw new RuntimeException(Config.HINT);
        }
        if (!f.exists()) {
            throw new RuntimeException("config file " + f.getAbsolutePath() + " not found");
        }
        try {
            InputStream in = new FileInputStream(f);
            InputStreamReader reader = new InputStreamReader(in, encoding);
            BufferedReader br = new BufferedReader(reader);
            String line = null;

            content.clear();
            entries.clear();
            linemap.clear();
            int lineno = 0;
            while ((line = br.readLine()) != null) {
                if (line.startsWith("#") || line.startsWith("%")) {
                    /* this is comment */
                } else if (line.matches("^[ \t]*$")) {
                    /* this is empty line */
                } else {
                    if (!line.contains("=")) {
                        log.warn("invalid line {" + line + "}");
                    } else {
                        String[] sub = line.split("=");
                        if (sub.length == 2) {
                            entries.put(sub[0], sub[1]);
                            linemap.put(new Integer(lineno), sub[0]);
                        } else if (sub.length == 1) {
                            entries.put(sub[0], "");
                            linemap.put(new Integer(lineno), sub[0]);
                        } else {
                            log.warn("invalid line {" + line + "}");
                        }
                    }
                }
                content.add(line);
                ++lineno;
            }

            br.close();
            reader.close();
            in.close();
        }
        catch(IOException e) {
            e.printStackTrace();
        }
    }

    public static void save() {
        if (inifile == null) {
            throw new RuntimeException(Config.HINT);
        }
        try {
            FileOutputStream fos = new FileOutputStream(inifile);
            Writer out = new OutputStreamWriter(fos, encoding);
            String eol = "\r\n";
            for (int i = 0; i < content.size(); i++) {
                String key = linemap.get(new Integer(i));
                if (key != null) {
                    out.write(key + "=" + entries.get(key) + eol);
                } else {
                    out.write(content.get(i) + eol);
                }
            }
            for (String key : new_entries.keySet()) {
                out.write(key + "=" + new_entries.get(key) + eol);
            }
            out.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static int hexInt(String str) {
        int val = 0;
        if (str.matches("^[0-9a-fA-F]+$")) {
            for (int i = 0; i < str.length(); i++) {
                char c = str.charAt(i);
                int v = 0;
                if ('0' <= c && c <= '9') v = c - '0';
                else if ('a' <= c && c <= 'f') v = c - 'a' + 10;
                else if ('A' <= c && c <= 'F') v = c - 'A' + 10;
                val = (val << 4) + v;
            }
        }
        return val;
    }

}
