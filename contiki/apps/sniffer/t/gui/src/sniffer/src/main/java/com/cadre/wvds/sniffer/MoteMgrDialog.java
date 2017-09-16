package com.cadre.wvds.sniffer;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.List;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.RowFilter;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

import net.skaldchen.commons.utils.BytesUtil;

@SuppressWarnings("serial")
public class MoteMgrDialog extends JDialog implements ActionListener {

	private JTable table;
	private DefaultTableModel tableM;
	private TableRowSorter<TableModel> rowSorter;
	private JTextField filterInput;

	public MoteMgrDialog(JFrame parent) {
		super(parent, true);
		setTitle("节点管理器");
		setSize(400, 320);
		setDefaultCloseOperation(JDialog.HIDE_ON_CLOSE);

		JPanel panel = new JPanel();
		getContentPane().add(panel, BorderLayout.NORTH);

		JLabel filterPre = new JLabel("过滤:");
		panel.add(filterPre);

		filterInput = new JTextField();
		filterInput.setColumns(10);
		panel.add(filterInput);
		filterInput.setPreferredSize(new Dimension(40,20));

		filterInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				setFilter();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				setFilter();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				setFilter();
			}
		});

		JLabel sepLabel = new JLabel();
		panel.add(sepLabel);
		sepLabel.setPreferredSize(new Dimension(20,18));

		JButton refrButton = new JButton("刷新");
		panel.add(refrButton);
		refrButton.setMargin(new Insets(1,1,1,1));
		refrButton.setPreferredSize(new Dimension(50,22));
		refrButton.setActionCommand("refresh");
		refrButton.addActionListener(this);

		JButton addButton = new JButton("增加");
		panel.add(addButton);
		addButton.setMargin(new Insets(1,1,1,1));
		addButton.setPreferredSize(new Dimension(50,22));
		addButton.setActionCommand("add");
		addButton.addActionListener(this);

		JButton remButton = new JButton("删除");
		panel.add(remButton);
		remButton.setMargin(new Insets(1,1,1,1));
		remButton.setPreferredSize(new Dimension(50,22));
		remButton.setActionCommand("remove");
		remButton.addActionListener(this);

		JButton saveButton = new JButton("保存");
		panel.add(saveButton);
		saveButton.setMargin(new Insets(1,1,1,1));
		saveButton.setPreferredSize(new Dimension(50,22));
		saveButton.setActionCommand("save");
		saveButton.addActionListener(this);

		JScrollPane panel_1 = new JScrollPane();
		getContentPane().add(panel_1, BorderLayout.CENTER);

		String[] HEADER = new String[] { "车位号/节点名", "设备编码" };
		tableM = new DefaultTableModel(HEADER, 0) {
			@Override
			public boolean isCellEditable(int row, int column) {
				if (column == 1)
					return false;
				return super.isCellEditable(row, column);
			}
		};
		table = new JTable();
		table.setModel(tableM);
		panel_1.setViewportView(table);

		rowSorter = new TableRowSorter<TableModel>(tableM);
		table.setRowSorter(rowSorter);
	}

	public void load() {
		List<Mote> motes = MoteManager.getInstance().getMotes();
		tableM.setRowCount(0);
		for (Mote m : motes)
			addMote(m);
	}

	private void addMote(Mote m) {
		Object[] row = new Object[3];
		int i = 0;
		row[i++] = m.getName();
		row[i++] = Mote.strDev(m.getDev());
		tableM.addRow(row);
	}

	private Mote rowToMote(int r) {
		Mote m = null;
		if (r >= 0 && r < table.getRowCount()) {
			String sDev = (String)table.getValueAt(r, 1);
			byte[] dev = BytesUtil.toBytes(sDev);
			m = MoteManager.getInstance().getMoteByDev(dev);
		}
		return m;
	}

	private void setFilter() {
		String filter = filterInput.getText();
		rowSorter.setRowFilter(RowFilter.regexFilter(filter));
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		String action = e.getActionCommand();
		if (action.equals("add")) {
			Object[] row = new Object[4];
			tableM.addRow(row);

			int r = table.getRowCount() - 1;
			Rectangle rect = table.getCellRect(r, 0, true);
			table.scrollRectToVisible(rect);
			table.getSelectionModel().setSelectionInterval(r, r);
		}
		else if (action.equals("remove")) {
			int[] rows = table.getSelectedRows();
			String message = String.format("确定删除选中的%d个节点吗？", rows.length);
			int r = JOptionPane.showConfirmDialog(this, message, "确认",0,3);
			if (r == JOptionPane.YES_OPTION) {
				MoteManager mgr = MoteManager.getInstance();
				for (int i = rows.length-1; i >= 0; i--) {
					Mote m = rowToMote(rows[i]);
					mgr.removeMote(m);
					tableM.removeRow(rows[i]);
				}
				mgr.save();
			}
		}
		else if (action.equals("save")) {
			MoteManager mgr = MoteManager.getInstance();
			for (int i = 0; i < table.getRowCount(); i++) {
				Mote m = rowToMote(i);
				m.setName((String)table.getValueAt(i, 0));
			}
			mgr.save();
		}
		else if (action.equals("refresh")) {
			load();
		}
		else {
			System.err.println("MoteMgrDialog: not support action " + action);
		}
	}

}
