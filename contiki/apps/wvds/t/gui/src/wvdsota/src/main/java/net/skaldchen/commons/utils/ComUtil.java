package net.skaldchen.commons.utils;

import gnu.io.CommPortIdentifier;
import gnu.io.NoSuchPortException;
import gnu.io.SerialPort;

public class ComUtil {

	public static boolean exist(String comName) {
		boolean ok = false;

		try {
			CommPortIdentifier.getPortIdentifier(comName);
			ok = true;
		} catch (NoSuchPortException e) {
			System.err.println("" + comName + " not exist: " + e.getMessage());
		}

		return ok;
	}

	public static boolean avail(String comName) {
		SerialPort com = null;
		boolean ok = false;

		try {
			CommPortIdentifier ident = CommPortIdentifier.getPortIdentifier(comName);
			com = (SerialPort) ident.open("ComReader", 2000);
			ok = true;
		} catch (Exception e) {
			System.err.println("" + comName + " not avail: " + e.getMessage());
		} finally {
			if (com != null)
				com.close();
		}

		return ok;
	}

}
