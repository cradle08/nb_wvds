package com.cadre.wvds.sniffer;

import java.awt.Color;
import java.awt.Component;

import javax.swing.JTable;
import javax.swing.table.DefaultTableCellRenderer;

public class VDStateCellRenderer extends DefaultTableCellRenderer
{
	public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column)
	{
		Component c = super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);
		if (value instanceof String) {
			if (((String)value).equals(WVDS.strState(0))) { // 无车
				setBackground(Color.green);
				setForeground(Color.black);
			} else if (((String)value).equals(WVDS.strState(1))) { // 有车
				setBackground(Color.red);
				setForeground(Color.white);
			} else if (((String)value).equals(WVDS.strState(3))) { // 标定中
				setBackground(Color.yellow);
				setForeground(Color.black);
			} else if (((String)value).equals(WVDS.strState(2))) { // 待激活
				setBackground(Color.pink);
				setForeground(Color.black);
			} else {
				setBackground(null);
				setForeground(Color.black);
			}
		}
		return c;
	}
}
