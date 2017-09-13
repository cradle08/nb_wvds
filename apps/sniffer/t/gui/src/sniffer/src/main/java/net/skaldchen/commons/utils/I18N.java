package net.skaldchen.commons.utils;

import java.util.MissingResourceException;
import java.util.ResourceBundle;

public class I18N {

	public static void init(String fname) {
		if (fname != null && fname.length() > 0) {
			try {
				rb = ResourceBundle.getBundle(fname);
			} catch (MissingResourceException e) {
				System.err.println("I18N: translation file for " + fname + " not found");
			}
		} else {
			System.err.println("I18N: cannot initialize with <" + fname + ">");
		}
	}

	public static String get(String name) {
		String result = "N/A";
		try {
			if (rb != null)
				result = rb.getString(name);
		} catch (MissingResourceException e) {
			System.err.println("warning: no translation entry for " + name);
		}
		return result;
	}

	public static String get(String name, String dflt) {
		String result = dflt;
		try {
			if (rb != null)
				result = rb.getString(name);
		} catch (MissingResourceException e) {}
		return result;
	}

	public static boolean has(String name) {
		boolean result = false;
		try {
			if (rb != null) {
				rb.getString(name);
				result = true;
			}
		} catch (MissingResourceException e) { }
		return result;
	}

	private static ResourceBundle rb = null;

}
