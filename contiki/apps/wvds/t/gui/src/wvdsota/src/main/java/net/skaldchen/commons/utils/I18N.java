package net.skaldchen.commons.utils;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

public class I18N {

	public static void init() {
		init(null);
	}

	public static void init(String fname) {
		if (fname != null) rbname = fname;
		rb = ResourceBundle.getBundle(rbname);
	}

	public static String get(String name) {
		String result = "";
		try {
			result = rb.getString(name);
		} catch (MissingResourceException e) {
			System.err.println("warning: no translation entry for " + name);
			result = "N/A";
		}
		return result;
	}

	public static String get(String name, String dflt) {
		String result;
		try {
			result = rb.getString(name);
		} catch (MissingResourceException e) {
			result = dflt;
		}
		return result;
	}

	public static String[] get(String pre, String[] names) {
		String[] result = new String[names.length];
		for (int i = 0; i < names.length; i++)
			result[i] = get(pre + names[i]);
		return result;
	}

	public static boolean has(String name) {
		boolean result = false;
		try {
			rb.getString(name);
			result = true;
		} catch (MissingResourceException e) { }
		return result;
	}

	private static String rbname = null;
	private static ResourceBundle rb = null;

}
