package net.skaldchen.commons.utils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.dom4j.io.OutputFormat;
import org.dom4j.io.SAXReader;
import org.dom4j.io.XMLWriter;
import org.dom4j.DocumentException;
import org.dom4j.DocumentHelper;
import org.dom4j.Element;
import org.dom4j.Document;

public class Dom4JUtil {

	public static Document createXML(String xmlfile, String root) {
		Document doc = null;
		try {
			File file1 = new File(xmlfile);
			if (file1.exists()) {
				debug("remove existing file first");
				file1.delete();
			}
			file1.createNewFile();

			doc = DocumentHelper.createDocument();
			doc.addElement(root);

			writeXML(doc, xmlfile);
		} catch (IOException e) {
			e.printStackTrace();
		}
		return doc;
	}

	public static Document readXML(String xmlfile) {
		Document doc = null;
		try {
			File f = new File(xmlfile);
			if (f.exists()) {
				SAXReader reader = new SAXReader();
				doc = reader.read(f);
			} else {
				System.err.println("not found xml file " + xmlfile);
			}
		} catch (DocumentException e) {
			e.printStackTrace();
		}
		return doc;
	}

	public static void writeXML(Document document, String xmlfile) {
		try {
			OutputFormat format = OutputFormat.createPrettyPrint();
			format.setEncoding("utf-8");
			XMLWriter writer = new XMLWriter(new PrintWriter(new OutputStreamWriter(
					new FileOutputStream(xmlfile), "utf-8")), format);
			writer.write(document);
			writer.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public static ArrayList<Element> getElems(String xmlfile, String path) {
		Document doc = readXML(xmlfile);
		return getElems(doc, path);
	}

	public static ArrayList<Element> getElems(Document doc, String path) {
		ArrayList<Element> elems = new ArrayList<Element>();

		List<Element> list = doc.selectNodes(path);
		Iterator<Element> iter = list.iterator();
		while (iter.hasNext()) {
			Element elem = (Element)iter.next();
			elems.add(elem);
		}

		return elems;
	}

	public static Element getElem(Document doc, String path) {
		Element elem = null;

		List<Element> list = doc.selectNodes(path);
		Iterator<Element> iter = list.iterator();
		while (iter.hasNext()) {
			Element e = (Element)iter.next();
			elem = e; break;
		}

		return elem;
	}

	public static Element getElem(String xmlfile, String path, String tag, String val) {
		Document doc = readXML(xmlfile);
		return getElem(doc, path, tag, val);
	}

	public static Element getElem(Document doc, String path, String tag, String val) {
		Element elem = null;

		List<Element> list = doc.selectNodes(path);
		Iterator<Element> iter = list.iterator();
		while (iter.hasNext()) {
			Element e = (Element)iter.next();
			if (e.elementText(tag).equals(val)) {
				elem = e; break;
			}
		}

		return elem;
	}

	public static Element getElemByAttr(String xmlfile, String path, String attr, String val) {
		Document doc = readXML(xmlfile);
		return getElemByAttr(doc, path, attr, val);
	}

	public static Element getElemByAttr(Document doc, String path, String attr, String val) {
		Element elem = null;

		List<Element> list = doc.selectNodes(path);
		Iterator<Element> iter = list.iterator();
		while (iter.hasNext()) {
			Element e = (Element)iter.next();
			if (e.attributeValue(attr).equals(val)) {
				elem = e; break;
			}
		}

		return elem;
	}

	public static Element addElem(String xmlfile, String path, String name, HashMap<String,String> args) {
		Document doc = readXML(xmlfile);
		Element e = addElem(doc, path, name, args);
		writeXML(doc, xmlfile);
		return e;
	}
	
	public static Element addElem(Document doc, String path, String name, HashMap<String,String> args) {
		Element rval = null;

		List<Element> list = doc.selectNodes(path);
		Iterator<Element> iter = list.iterator();
		while (iter.hasNext()) {
			Element elem = (Element)iter.next();
			debug("Dom4JUtil: add node <" + name + "> under " + path);
			Element sub = elem.addElement(name);
			for (String key : args.keySet()) {
				String text = args.get(key);
				if (text == null) text = "";
				debug("" + key + " = <" + args.get(key) + ">");
				sub.addElement(key).setText(text);
			}
			rval = sub;
		}
		return rval;
	}

	public static boolean updateElem(String xmlfile, String path, String tag, String val,
			HashMap<String,String> args) {
		boolean rval = false;
		try {
			Document document = readXML(xmlfile);

			List<Element> list = document.selectNodes(path);
			Iterator<Element> iter = list.iterator();
			while (iter.hasNext()) {
				Element elem = (Element)iter.next();
				if (elem.elementText(tag).equals(val)) {
					debug("Dom4JUtil: update " + path + " where tag " + tag + "=" + val + " in " + xmlfile);
					for (String name : args.keySet()) {
						debug(name + " = <" + args.get(name) + ">");
						String text = args.get(name);
						if (text == null) text = "";
						Element e = elem.element(name);
						if (e == null) e = elem.addElement(name);
						e.setText(text);
					}
					rval = true;
					break;
				}
			}

			writeXML(document, xmlfile);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return rval;
	}

	public static boolean updateElemByAttr(String xmlfile, String path, String attr, String val,
			HashMap<String,String> args) {
		boolean rval = false;
		try {
			Document document = readXML(xmlfile);

			List<Element> list = document.selectNodes(path);
			Iterator<Element> iter = list.iterator();
			while (iter.hasNext()) {
				Element elem = (Element)iter.next();
				if (elem.attributeValue(attr).equals(val)) {
					debug("Dom4JUtil: update " + path + " where attr " + attr + "=" + val + " in " + xmlfile);
					for (String name : args.keySet()) {
						elem.element(name).setText(args.get(name));
						debug(name + " = " + args.get(name));
					}
					rval = true;
					break;
				}
			}

			writeXML(document, xmlfile);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return rval;
	}

	public static boolean removeElem(String xmlfile, String path, String tag, String val) {
		boolean success = false;
		try {
			Document document = readXML(xmlfile);

			List<Element> list = document.selectNodes(path);
			Iterator<Element> iter = list.iterator();
			while (iter.hasNext()) {
				Element elem = (Element)iter.next();
				if (elem.elementText(tag).equals(val)) {
					elem.detach();
					success = true;
					debug("Dom4JUtil: remove tag " + tag + "=" + val + " from " + path + " in " + xmlfile);
					break;
				}
			}

			writeXML(document, xmlfile);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return success;
	}

	public static boolean removeElemByAttr(String xmlfile, String path, String attr, String val) {
		boolean success = false;
		try {
			Document document = readXML(xmlfile);

			List<Element> list = document.selectNodes(path);
			Iterator<Element> iter = list.iterator();
			while (iter.hasNext()) {
				Element elem = (Element)iter.next();
				if (elem.attributeValue(attr).equals(val)) {
					elem.detach();
					success = true;
					debug("Dom4JUtil: remove attr " + attr + "=" + val + " from " + path + " in " + xmlfile);
					break;
				}
			}

			writeXML(document, xmlfile);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return success;
	}

	private static void debug(String s) {
		//System.out.println(s);
	}
}
