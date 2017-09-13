package net.skaldchen.commons.utils;

import java.awt.event.ActionListener;

import javax.swing.ButtonGroup;
import javax.swing.Icon;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JRadioButtonMenuItem;
import javax.swing.JSeparator;
import javax.swing.KeyStroke;

public class MenuUtil {

	public static final Boolean bT = new Boolean(true);
	public static final Boolean bF = new Boolean(false);

	static class MType {
		MType(int i) {
		}
	};

	public static final MType mi = new MType(1); // Normal menu item
	public static final MType cb = new MType(2); // Checkbox menu item
	public static final MType rb = new MType(3); // Radio button menu item

	static ButtonGroup bgroup;

	public static JMenuBar createMenuBar(Object[] menuBarData) {
		JMenuBar menuBar = new JMenuBar();
		for (int i = 0; i < menuBarData.length; i++)
			menuBar.add(createMenu((Object[][]) menuBarData[i]));
		return menuBar;
	}

	public static JMenu createMenu(Object[][] menuData) {
		JMenu menu = new JMenu();
		menu.setText(I18N.get("menu." + (String) menuData[0][0]));
		menu.setMnemonic(((Character) menuData[0][1]).charValue());
		// Create redundantly, in case there are
		// any radio buttons:
		bgroup = new ButtonGroup();
		for (int i = 1; i < menuData.length; i++) {
			if (menuData[i][0] == null) // separator
				menu.add(new JSeparator());
			else if (menuData[i][0].getClass().isArray()) // submenu
				menu.add(createMenu((Object[][])menuData[i][0]));
			else // menu item
				menu.add(createMenuItem(menuData[i]));
		}
		return menu;
	}

	public static JMenuItem findMenuItem(JMenuBar menubar, String key) {
		debug("findMenuItem: " + key);
		JMenuItem rval = null;
		for (int i = 0; i < menubar.getComponentCount(); i++) {
			JMenu menu = (JMenu) menubar.getComponent(i);
			debug("findMenuItem: checking menu " + menu.getActionCommand() + ", "
					+ i + "/" + menubar.getComponentCount() + ", " + menu.getComponentCount() + " items");
			for (int j = 0; j < menu.getComponentCount(); j++) {
				JMenuItem item = (JMenuItem) menu.getComponent(j);
				debug("findMenuItem: checking item " + item.getActionCommand() + ", " + j + "/" + menu.getComponentCount());
				if (item.getActionCommand().equals(key)) {
					debug("findMenuItem: found " + item.getActionCommand());
					rval = item; break;
				}
			}
			if (rval != null) break;
		}
		return rval;
	}

	// data[0]: menu item name, also as action command
	// data[1]: menu item type
	// data[2]: action listener
	// data[3]: enabled, optional, default Boolean.TRUE
	// data[4]: mnemonic, optional
	// data[5]: accelerator, optional
	// data[6]: selected, optional
	// data[7]: icon, optional
	public static JMenuItem createMenuItem(Object[] data) {
		String name = (String) data[0];
		MType type = (MType) data[1];
		ActionListener listener = (ActionListener) data[2];
		Boolean enable = (data.length > 3) ? (Boolean) data[3] : Boolean.TRUE;
		Character mnemonic = (data.length > 4) ? (Character) data[4] : null;
		String accel = (data.length > 5) ?(String)data[5] : null;
		Boolean selected = (data.length > 6) ? (Boolean) data[6] : null;
		Icon icon = (data.length > 7) ? (Icon) data[7] : null;

		JMenuItem m = null;
		if (type == mi)
			m = new JMenuItem();
		else if (type == cb)
			m = new JCheckBoxMenuItem();
		else if (type == rb) {
			m = new JRadioButtonMenuItem();
			bgroup.add(m);
		}
		m.setText(I18N.get("menu." + name));
		m.setActionCommand("menu." + name);
		m.addActionListener(listener);
		m.setEnabled(enable.booleanValue());
		if (mnemonic != null)
			m.setMnemonic(mnemonic.charValue());
		if (accel != null && accel.trim().length() > 0)
			m.setAccelerator(KeyStroke.getKeyStroke(accel));
		if (selected != null && (type == cb || type == rb))
			m.setSelected(selected.booleanValue());
		if (icon != null)
			m.setIcon(icon);
		return m;
	}

	private static void debug(String s) {
		System.out.println("MenuUtil: " + s);
	}
}
