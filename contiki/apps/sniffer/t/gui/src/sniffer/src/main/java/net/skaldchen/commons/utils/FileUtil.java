package net.skaldchen.commons.utils;

import java.io.*;
import java.util.ArrayList;
import java.util.Collections;

public class FileUtil {

    private static String encoding = "UTF-8";

	public static String fixBlank(String filename) {
		String str = "";
		for (int i = 0; i < filename.length(); i++) {
			char c = filename.charAt(i);
			if (c == ' ') str += "\"" + c + "\"";
			else str += c;
		}
		return str;
	}

	public static void sortFile(String file) {
		sortFile(file, file);
	}

	public static void sortFile(String infile, String outfile) {
		ArrayList<String> lines = new ArrayList<String>();

		try {
			// read contents of input file
			BufferedReader in = new BufferedReader(new FileReader(infile));
			while (in.ready()) {
				String line = in.readLine();
				lines.add(line);
			}
			in.close();

			// sort the lines
			Collections.sort(lines);

			// write out into output file
			FileWriter out = new FileWriter(outfile);
			for (String line : lines) {
				out.write(line + "\n");
			}
			out.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

    public static StringBuilder readAsBuffer(String fname) {
        return readAsBuffer(new File(fname));
    }

    public static StringBuilder readAsBuffer(File f) {
        StringBuilder sb = new StringBuilder();
        try {
            InputStream in = new FileInputStream(f);
            InputStreamReader reader = new InputStreamReader(in, encoding);

            String inStr;
            BufferedReader br = new BufferedReader(reader);
            while ((inStr = br.readLine()) != null)
                sb.append(inStr);
        } catch(IOException e) {
            e.printStackTrace();
        }
        return sb;
    }

	// read in a text file as a string
    public static String readAsString(String fname) {
        return readAsString(new File(fname));
    }

	public static String readAsString(File f) {
		StringBuilder sb = new StringBuilder();
		try {
			InputStream in = new FileInputStream(f);
			InputStreamReader reader = new InputStreamReader(in, encoding);

			String inStr;
			BufferedReader br = new BufferedReader(reader);
			while ((inStr = br.readLine()) != null)
				sb.append(inStr);
		} catch(IOException e) {
			e.printStackTrace();
		}
		return sb.toString();
	}

    public static String readResAsString(String fname) {
        StringBuilder sb = new StringBuilder();

        try {
            InputStream is = FileUtil.class.getResourceAsStream(fname);
            InputStreamReader isr = new InputStreamReader(is, encoding);
            BufferedReader br = new BufferedReader(isr);
            String line = null;
            while ((line = br.readLine()) != null) {
                sb.append(line);
            }
            br.close();
            isr.close();
            is.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return sb.toString();
    }

	public static void writeToFile(String tofile, String text) {
		try {
			FileOutputStream fos = new FileOutputStream(tofile);
            Writer out = new OutputStreamWriter(fos, encoding);
            out.write(text);
			out.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

    public static String relPath(String dir, String absFile) {
        String relFile = absFile;
        String dirName = new File(dir).getAbsolutePath();
        dirName = dirName.replaceAll("[.]$", "");
        if (absFile.indexOf(dirName) != -1) {
            relFile = absFile.substring(dirName.length());
            relFile = relFile.replace('\\', '/');
        }
        return relFile;
    }
}
