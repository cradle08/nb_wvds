package com.cadre.wvds.sniffer;

import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.RowFilter;
import javax.swing.border.TitledBorder;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.packet.Packet;
import net.skaldchen.commons.serial.PacketListener;
import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.JTableUtil;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;

public class TabAnalyse extends JPanel implements PacketListener, ActionListener {

	private static final Logger log = LoggerFactory.getLogger(TabAnalyse.class);

	public JSplitPane splitPane1;
	public JSplitPane splitPane2;
	private JTable APTable;
	private JTable RPTable;
	private JTable VDTable;
	private DefaultTableModel APTableM;
	private DefaultTableModel RPTableM;
	private DefaultTableModel VDTableM;
	private JTextField RPFilterInput;
	private JTextField VDFilterInput;

	private String[] APTableH = new String[]{"�豸����", "״̬", "GPRS״̬", "GPRS�ź�", "��������", "���ߴ���", "��ע"};
	private int[] APTableW = new int[]{70,60,40,40,40,40,390};
	private String[] RPTableH = new String[]{"�豸����", "״̬", "�̵�ַ", "��������", "�������", "���(V)", "���(V)", "��������", "��ע"};
	private int[] RPTableW = new int[]{70,60,30,40,40,30,40,40,300};
	private String[] VDTableH = new String[]{"�豸����", "״̬", "״̬����", "��Ƶ", "��λ��", "�̵�ַ", "��������", "�������", "�������", "�������", "��������", "���(V)", "��ע"};
	private int[] VDTableW = new int[]{70,60,40,30,40,30,40,40,40,40,40,30,70};
	private TableRowSorter<TableModel> RPRowSorter;
	private TableRowSorter<TableModel> VDRowSorter;

	private final int COL_AP_DEVMAC = 0;
	private final int COL_AP_STATUS = 1;
	private final int COL_AP_GPRS_STATUS = 2;
	private final int COL_AP_GPRS_RSSI = 3;
	private final int COL_AP_BOOT_N = 4;
	private final int COL_AP_OFFLINE_N = 5;
	private final int COL_AP_REMARK = 6;

	private final int COL_RP_DEVMAC = 0;
	private final int COL_RP_STATUS = 1;
	private final int COL_RP_SADDR = 2;
	private final int COL_RP_HBEAT_N = 3;
	private final int COL_RP_HBEAT_LAST = 4;
	private final int COL_RP_BATV = 5;
	private final int COL_RP_SOLAR = 6;
	private final int COL_RP_BOOT_N = 7;
	private final int COL_RP_REMARK = 8;

	private final int COL_VD_DEVMAC = 0;
	private final int COL_VD_STATUS = 1;
	private final int COL_VD_STATUS_LAST = 2;
	private final int COL_VD_RADIO = 3;
	private final int COL_VD_PARKLOT = 4;
	private final int COL_VD_SADDR = 5;
	private final int COL_VD_HBEAT_N = 6;
	private final int COL_VD_HBEAT_LAST = 7;
	private final int COL_VD_CMDREQ_N = 8;
	private final int COL_VD_CMDREQ_LAST = 9;
	private final int COL_VD_BOOT_N = 10;
	private final int COL_VD_BATV = 11;
	private final int COL_VD_REMARK = 12;

	private final DecimalFormat voltFmt = new DecimalFormat("0.0");

	public TabAnalyse() {
		setLayout(new BorderLayout());

		splitPane1 = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
		add(splitPane1, BorderLayout.CENTER);

		JPanel panel = new JPanel();
		splitPane1.add(panel, JSplitPane.TOP);
		panel.setLayout(new BorderLayout());
		panel.setBorder(new TitledBorder("���ؽڵ�(AP)"));

		JScrollPane scrollPane = new JScrollPane();
		panel.add(scrollPane, BorderLayout.CENTER);

		APTableM = new DefaultTableModel(APTableH, 0);
		APTable = new JTable(APTableM);
		scrollPane.setViewportView(APTable);
		APTable.setRowHeight(20);
		JTableUtil.fitTableColumns(APTable, APTableW);

		JPanel panel_e = new JPanel();
		panel.add(panel_e, BorderLayout.EAST);
		panel_e.setPreferredSize(new Dimension(70,60));
		panel_e.setLayout(null);

		JButton removeAPBtn = new JButton("ɾ��");
		panel_e.add(removeAPBtn);
		removeAPBtn.setBounds(5,0,60,22);
		removeAPBtn.setActionCommand("removeAP");
		removeAPBtn.addActionListener(this);

		JButton clearAPBtn = new JButton("���");
		//panel_e.add(clearAPBtn);
		clearAPBtn.setBounds(5,25,60,22);
		clearAPBtn.setActionCommand("clearAP");
		clearAPBtn.addActionListener(this);

		splitPane2 = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
		splitPane1.add(splitPane2, JSplitPane.BOTTOM);

		JPanel panel_1 = new JPanel();
		splitPane2.add(panel_1, JSplitPane.TOP);
		panel_1.setLayout(new BorderLayout());
		panel_1.setBorder(new TitledBorder("�м���(RP)"));

		JScrollPane scrollPane_1 = new JScrollPane();
		panel_1.add(scrollPane_1, BorderLayout.CENTER);

		RPTableM = new DefaultTableModel(RPTableH, 0);
		RPTable = new JTable(RPTableM);
		scrollPane_1.setViewportView(RPTable);
		RPTable.setRowHeight(20);
		JTableUtil.fitTableColumns(RPTable, RPTableW);

		RPRowSorter = new TableRowSorter<TableModel>(RPTableM);
		RPTable.setRowSorter(RPRowSorter);

		JPanel panel_1e = new JPanel();
		panel_1.add(panel_1e, BorderLayout.EAST);
		panel_1e.setPreferredSize(new Dimension(70,100));
		panel_1e.setLayout(null);

		JLabel RPFilterLbl = new JLabel("����:");
		panel_1e.add(RPFilterLbl);
		RPFilterLbl.setBounds(5,4,60,18);

		RPFilterInput = new JTextField();
		panel_1e.add(RPFilterInput);
		RPFilterInput.setBounds(6,25,58,20);
		RPFilterInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				setRPFilter();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				setRPFilter();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				setRPFilter();
			}
		});

		JButton removeRPBtn = new JButton("ɾ��");
		panel_1e.add(removeRPBtn);
		removeRPBtn.setBounds(5,50,60,22);
		removeRPBtn.setActionCommand("removeRP");
		removeRPBtn.addActionListener(this);

		JButton clearRPBtn = new JButton("���");
		//panel_1e.add(clearRPBtn);
		clearRPBtn.setBounds(5,75,60,22);
		clearRPBtn.setActionCommand("clearRP");
		clearRPBtn.addActionListener(this);

		JPanel panel_2 = new JPanel();
		splitPane2.add(panel_2, JSplitPane.BOTTOM);
		panel_2.setLayout(new BorderLayout());
		panel_2.setBorder(new TitledBorder("��λ�����(VD)"));

		JScrollPane scrollPane_2 = new JScrollPane();
		panel_2.add(scrollPane_2, BorderLayout.CENTER);

		VDTableM = new DefaultTableModel(VDTableH, 0);
		VDTable = new JTable(VDTableM);
		scrollPane_2.setViewportView(VDTable);
		VDTable.setRowHeight(20);
		JTableUtil.fitTableColumns(VDTable, VDTableW);

		VDRowSorter = new TableRowSorter<TableModel>(VDTableM);
		VDTable.setRowSorter(VDRowSorter);

		TableColumn stateCol = VDTable.getColumnModel().getColumn(COL_VD_STATUS);
		stateCol.setCellRenderer(new VDStateCellRenderer());

		TableColumn radioCol = VDTable.getColumnModel().getColumn(COL_VD_RADIO);
		radioCol.setCellRenderer(new VDRadioCellRenderer());

		JPanel panel_2e = new JPanel();
		panel_2.add(panel_2e, BorderLayout.EAST);
		panel_2e.setPreferredSize(new Dimension(70,100));
		panel_2e.setLayout(null);

		JLabel VDFilterLbl = new JLabel("����:");
		panel_2e.add(VDFilterLbl);
		VDFilterLbl.setBounds(5,4,60,18);

		VDFilterInput = new JTextField();
		panel_2e.add(VDFilterInput);
		VDFilterInput.setBounds(6,25,58,20);
		VDFilterInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				setVDFilter();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				setVDFilter();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				setVDFilter();
			}
		});

		JButton removeVDBtn = new JButton("ɾ��");
		panel_2e.add(removeVDBtn);
		removeVDBtn.setBounds(5,50,60,22);
		removeVDBtn.setActionCommand("removeVD");
		removeVDBtn.addActionListener(this);

		JButton clearVDBtn = new JButton("���");
		//panel_2e.add(clearVDBtn);
		clearVDBtn.setBounds(5,75,60,22);
		clearVDBtn.setActionCommand("clearVD");
		clearVDBtn.addActionListener(this);

		load();
	}

	private void load() {
		List<Mote> motes = MoteManager.getInstance().getMotes();
		for (Mote m : motes) {
			if (m.getDev()[0] == WVDS.VD) {
				addVD(m);
			}
			else if (m.getDev()[0] == WVDS.RP) {
				addRP(m);
			}
			else if (m.getDev()[0] == WVDS.AP) {
				addAP(m);
			}
		}
	}

	private void setRPFilter() {
		String filter = RPFilterInput.getText();
		RPRowSorter.setRowFilter(RowFilter.regexFilter(filter));
	}

	private void setVDFilter() {
		String filter = VDFilterInput.getText();
		VDRowSorter.setRowFilter(RowFilter.regexFilter(filter));
	}

	private int rowVD(byte[] dev) {
		String sDev = WVDS.strDev(dev);
		for (int i = 0; i < VDTable.getRowCount(); i++) {
			if (sDev.equals((String)VDTable.getValueAt(i, COL_VD_DEVMAC))) {
				return i;
			}
		}
		return -1;
	}

	private Mote rowToVD(int row) {
		String sdev = (String) VDTable.getValueAt(row, COL_VD_DEVMAC);
		byte[] dev = BytesUtil.toBytes(sdev);
		return MoteManager.getInstance().getMoteByDev(dev);
	}

	private void addVD(Mote m) {
		Object[] rowData = new Object[VDTableH.length];
		int i = 0;
		rowData[i++] = WVDS.strDev(m.getDev());
		rowData[i++] = "";
		rowData[i++] = "";
		rowData[i++] = "";
		rowData[i++] = m.getName();
		rowData[i++] = Integer.valueOf(m.getSaddr());
		rowData[i++] = Integer.valueOf(0);
		rowData[i++] = "";
		rowData[i++] = Integer.valueOf(0);
		rowData[i++] = "";
		rowData[i++] = Integer.valueOf(0);
		rowData[i++] = voltFmt.format(0.0);
		rowData[i++] = "";
		VDTableM.addRow(rowData);
	}

	private int rowRP(byte[] dev) {
		String sDev = WVDS.strDev(dev);
		for (int i = 0; i < RPTable.getRowCount(); i++) {
			if (sDev.equals((String)RPTable.getValueAt(i, COL_RP_DEVMAC))) {
				return i;
			}
		}
		return -1;
	}

	private Mote rowToRP(int row) {
		String sdev = (String) RPTable.getValueAt(row, COL_RP_DEVMAC);
		byte[] dev = BytesUtil.toBytes(sdev);
		return MoteManager.getInstance().getMoteByDev(dev);
	}

	private void addRP(Mote m) {
		Object[] rowData = new Object[RPTableH.length];
		int i = 0;
		rowData[i++] = WVDS.strDev(m.getDev());
		rowData[i++] = "";
		rowData[i++] = Integer.valueOf(m.getSaddr());
		rowData[i++] = Integer.valueOf(0);
		rowData[i++] = "";
		rowData[i++] = voltFmt.format(0.0);
		rowData[i++] = voltFmt.format(0.0);
		rowData[i++] = Integer.valueOf(0);
		rowData[i++] = "";
		RPTableM.addRow(rowData);
	}

	private int rowAP(byte[] dev) {
		String sDev = WVDS.strDev(dev);
		for (int i = 0; i < APTable.getRowCount(); i++) {
			if (sDev.equals((String)APTable.getValueAt(i, COL_AP_DEVMAC))) {
				return i;
			}
		}
		return -1;
	}

	private Mote rowToAP(int row) {
		String sdev = (String) APTable.getValueAt(row, COL_AP_DEVMAC);
		byte[] dev = BytesUtil.toBytes(sdev);
		return MoteManager.getInstance().getMoteByDev(dev);
	}

	private void addAP(Mote m) {
		Object[] rowData = new Object[APTableH.length];
		int i = 0;
		rowData[i++] = WVDS.strDev(m.getDev());
		rowData[i++] = "";
		rowData[i++] = "";
		rowData[i++] = Integer.valueOf(0);
		rowData[i++] = Integer.valueOf(0);
		rowData[i++] = Integer.valueOf(0);
		rowData[i++] = "";
		APTableM.addRow(rowData);
	}

	public void clear() {
		APTableM.setRowCount(0);
		RPTableM.setRowCount(0);
		VDTableM.setRowCount(0);
	}

	private void focusRow(JTable table, int r) {
		Rectangle cell = table.getCellRect(r, 0, false);
		table.scrollRectToVisible(cell);
		table.getSelectionModel().setSelectionInterval(r, r);
	}

	private final SimpleDateFormat secFmt = new SimpleDateFormat("HH:mm:ss");

	private int reinitTo;
	private byte[] reinitDev;
	private int reinitSeq;

	public void addPacket(Packet p, Date rtime) {
		if (p == null)
			return;

		try {
			if ("park evt".equals(p.getName())) {
				byte[] dev = p.getField("dev").value;
				int status = p.getField("status").toInt();
				byte[] mac = new byte[8];
				System.arraycopy(dev, 0, mac, 2, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}
				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}
				VDTable.setValueAt(WVDS.strState(status), r, COL_VD_STATUS);
				VDTable.setValueAt(secFmt.format(rtime), r, COL_VD_STATUS_LAST);
				focusRow(VDTable, r);
			}
			else if ("vd hbeat".equals(p.getName())) {
				byte[] dev = p.getField("dev").value;
				int status = p.getField("status").toInt();
				double batv = p.getField("batV").toInt() * 0.1;
				byte[] mac = new byte[8];
				System.arraycopy(dev, 0, mac, 2, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}
				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}
				int addr = p.getField("esender").toInt();
				if (addr != m.getSaddr()) {
					m.setSaddr(addr);
					motedb.save();
					VDTable.setValueAt(Integer.valueOf(addr), r, COL_VD_SADDR);
				}
				int num = (Integer)VDTable.getValueAt(r, COL_VD_HBEAT_N); num += 1;
				VDTable.setValueAt(num, r, COL_VD_HBEAT_N);
				VDTable.setValueAt(secFmt.format(rtime), r, COL_VD_HBEAT_LAST);
				VDTable.setValueAt(WVDS.strState(status), r, COL_VD_STATUS);
				VDTable.setValueAt(secFmt.format(rtime), r, COL_VD_STATUS_LAST);
				VDTable.setValueAt(voltFmt.format(batv), r, COL_VD_BATV);
				focusRow(VDTable, r);
			}
			else if ("rp hbeat".equals(p.getName())) {
				byte[] dev = p.getField("dev").value;
				double batv = p.getField("batV").toInt() * 0.1;
				double solar = p.getField("solar").toInt() * 0.1;
				byte[] mac = new byte[8];
				System.arraycopy(dev, 0, mac, 2, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}
				int r = rowRP(dev);
				if (r == -1) {
					addRP(m);
					r = rowRP(dev);
				}
				int addr = p.getField("esender").toInt();
				if (addr != m.getSaddr()) {
					m.setSaddr(addr);
					motedb.save();
					RPTable.setValueAt(Integer.valueOf(addr), r, COL_RP_SADDR);
				}
				int num = (Integer)RPTable.getValueAt(r, COL_RP_HBEAT_N); num += 1;
				RPTable.setValueAt(num, r, COL_RP_HBEAT_N);
				RPTable.setValueAt(secFmt.format(rtime), r, COL_RP_HBEAT_LAST);
				RPTable.setValueAt(voltFmt.format(batv), r, COL_RP_BATV);
				RPTable.setValueAt(voltFmt.format(solar), r, COL_RP_SOLAR);
				focusRow(RPTable, r);
			}
			else if ("netcmd req".equals(p.getName())) {
				int from = p.getField("from").toInt();
				byte[] mac = p.getField("mac").value;
				byte[] dev = new byte[6];
				System.arraycopy(mac, 2, dev, 0, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}

				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}

				if (from != m.getSaddr()) {
					m.setSaddr(from);
					motedb.save();
					VDTable.setValueAt(m.getSaddr(), r, COL_VD_SADDR);
				}

				int num = (Integer)VDTable.getValueAt(r, COL_VD_CMDREQ_N);
				VDTable.setValueAt(num + 1, r, COL_VD_CMDREQ_N);
				VDTable.setValueAt(secFmt.format(rtime), r, COL_VD_CMDREQ_LAST);
				VDTable.setValueAt("�Ѽ���", r, COL_VD_RADIO);
				focusRow(VDTable, r);
			}
			else if ("log info".equals(p.getName())) {
				int nid = p.getField("from").toInt();
				int code = p.getField("code").toInt();

				Mote m = MoteManager.getInstance().getMote(nid);
				if (m != null) {
					int r = rowVD(m.getDev());
					if (r != -1) {
						if ((code == 0x91) || (code == 0x96) || (code == 0x93)) { // ��Ƶ�ѹر�
							VDTable.setValueAt("�ѹر�", r, COL_VD_RADIO);
							focusRow(VDTable, r);
						}
						else if ((code == 0x92)) { // �ȴ�����
							VDTable.setValueAt(WVDS.strState(2), r, COL_VD_STATUS);
							VDTable.setValueAt("�ѹر�", r, COL_VD_RADIO);
							VDTable.setValueAt("", r, COL_VD_REMARK);
							focusRow(VDTable, r);
						}
						else if (code == 0x94) { // ��Ƶ�ѿ���
							VDTable.setValueAt("�Ѽ���", r, COL_VD_RADIO);
							focusRow(VDTable, r);
						}
						else if (code == 0x82) { // �ָ���������
							VDTable.setValueAt(WVDS.strState(2), r, COL_VD_STATUS);
							VDTable.setValueAt("�ѻָ���������", r, COL_VD_REMARK);
							focusRow(VDTable, r);
						}
						else if (code == 0x83) { // �����ɹ�
							VDTable.setValueAt("�������ɹ�", r, COL_VD_REMARK);
							focusRow(VDTable, r);
						}
					}
				}
			}
			else if ("vd act req".equals(p.getName())) {
				int from = p.getField("from").toInt();
				byte[] dev = p.getField("dev").value;
				byte[] mac = new byte[8];
				System.arraycopy(dev, 0, mac, 2, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}

				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}

				if (from != m.getSaddr()) {
					m.setSaddr(from);
					motedb.save();
					VDTable.setValueAt(m.getSaddr(), r, COL_VD_SADDR);
				}

				VDTable.setValueAt(WVDS.strState(2), r, COL_VD_STATUS);
				VDTable.setValueAt("�Ѽ���", r, COL_VD_RADIO);
				int num = (Integer)VDTable.getValueAt(r, COL_VD_CMDREQ_N);
				VDTable.setValueAt(num + 1, r, COL_VD_CMDREQ_N);
				VDTable.setValueAt(secFmt.format(new Date()), r, COL_VD_CMDREQ_LAST);
				focusRow(VDTable, r);
			}
			else if ("fae reinit".equals(p.getName())) {
				reinitTo = p.getField("to").toInt();
				reinitDev = p.getField("dev").value;
				reinitSeq = p.getField("seqno").toInt();
			}
			else if ("netcmd ack".equals(p.getName())) {
				int from = p.getField("from").toInt();
				int to = p.getField("to").toInt();
				int seqno = p.getField("seqno").toInt();
				if (from == reinitTo && to == 258 && seqno == reinitSeq) {
					Mote m = MoteManager.getInstance().getMote(from);
					if (m != null) {
						int r = rowVD(m.getDev());
						if (r != -1) {
							VDTable.setValueAt(WVDS.strState(3), r, COL_VD_STATUS);
							VDTable.setValueAt("���ڽ��б궨", r, COL_VD_REMARK);
							focusRow(VDTable, r);
						}
					}
				}
			}
			else if ("vd act ack".equals(p.getName())) {
				byte[] dev = p.getField("dev").value;
				byte[] mac = new byte[8];
				System.arraycopy(dev, 0, mac, 2, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}

				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}
				VDTable.setValueAt("�����Ӽ�����", r, COL_VD_REMARK);
			}
			else if ("fae reinit resp".equals(p.getName())) {
				byte[] dev = p.getField("dev").value;
				byte[] mac = new byte[8];
				System.arraycopy(dev, 0, mac, 2, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}

				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}
				VDTable.setValueAt(WVDS.strState(0), r, COL_VD_STATUS);
				VDTable.setValueAt("�궨�����", r, COL_VD_REMARK);
			}
			else if ("fae disconn ack".equals(p.getName())) {
				byte[] dev = p.getField("dev").value;
				byte[] mac = new byte[8];
				System.arraycopy(dev, 0, mac, 2, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}

				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}
				VDTable.setValueAt("�ѹر�", r, COL_VD_RADIO);
				VDTable.setValueAt("�ѶϿ�������", r, COL_VD_REMARK);
			}
			else if ("reboot".equals(p.getName())) {
				byte[] mac = p.getField("mac").value;
				byte[] dev = new byte[6];
				System.arraycopy(mac, 2, dev, 0, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}
				if (dev[0] == WVDS.VD) {
					int r = rowVD(dev);
					if (r == -1) {
						addVD(m);
						r = rowVD(dev);
					}
					int num = (Integer)VDTable.getValueAt(r, COL_VD_BOOT_N);
					num += 1; VDTable.setValueAt(num, r, COL_VD_BOOT_N);
					focusRow(VDTable, r);
				}
				else if (dev[0] == WVDS.RP) {
					int r = rowRP(dev);
					if (r == -1) {
						addRP(m);
						r = rowRP(dev);
					}
					int num = (Integer)RPTable.getValueAt(r, COL_RP_BOOT_N);
					num += 1; RPTable.setValueAt(num, r, COL_RP_BOOT_N);
					focusRow(RPTable, r);
				}
				else if (dev[0] == WVDS.AP) {
					int r = rowAP(dev);
					if (r == -1) {
						addAP(m);
						r = rowAP(dev);
					}
					int num = (Integer)APTable.getValueAt(r, COL_AP_BOOT_N);
					num += 1; APTable.setValueAt(num, r, COL_AP_BOOT_N);
					focusRow(APTable, r);
				}
			}
			else if ("connect".equals(p.getName())) {
				int from = p.getField("from").toInt();
				byte[] dev = p.getField("dev").value;
				byte[] mac = new byte[8];
				System.arraycopy(dev, 0, mac, 2, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					m.setSaddr(from);
					motedb.addMote(m);
					motedb.save();
				}
				if (from != m.getSaddr()) {
					m.setSaddr(from);
					motedb.save();
				}

				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}
				VDTable.setValueAt(Integer.valueOf(from), r, COL_VD_SADDR);
			}
			else if ("accept".equals(p.getName())) {
				byte[] mac = p.getField("mac").value;
				byte[] dev = new byte[6];
				System.arraycopy(mac, 2, dev, 0, 6);

				MoteManager motedb = MoteManager.getInstance();
				Mote m = motedb.getMoteByDev(dev);
				if (m == null) {
					m = new Mote();
					m.setDev(dev);
					m.setMac(mac);
					m.setName("N/A");
					motedb.addMote(m);
					motedb.save();
				}

				int r = rowVD(dev);
				if (r == -1) {
					addVD(m);
					r = rowVD(dev);
				}
				//VDTable.setValueAt("������", r, COL_VD_REMARK);
			}
			else if ("gprs rssi".equals(p.getName())) {
				String str = p.getField("text").toStr();
				int dbm = Integer.parseInt(str.replaceAll("^[ \t]+", ""));
				APTable.setValueAt(Integer.valueOf(dbm), 0, COL_AP_GPRS_RSSI);
				focusRow(APTable, 0);
			}
		} catch(Exception e) {
			log.warn("fail add packet", e);
		}
	}

	public void addPacket(byte[] packet, Date rtime) {
		try {
			Packet p = Main.parser.parse(packet);
			addPacket(p, rtime);
		} catch(Exception e) {
			log.warn(String.format("fail add packet: %s, %s", secFmt.format(rtime), BytesUtil.strBytes(packet)), e);
		}
	}

	@Override
	public void packetReceived(byte[] packet) {
		addPacket(packet, new Date());
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		String act = e.getActionCommand();

		if ("clearVD".equals(act)) {
			VDTableM.setRowCount(0);
		}
		else if ("removeVD".equals(act)) {
			int[] rows = VDTable.getSelectedRows();
			String message = String.format("ȷ��ɾ��ѡ�е�%d��VD��", rows.length);
			int r = JOptionPane.showConfirmDialog(Main.gui, message, "ȷ��",0,3);
			if (r == JOptionPane.YES_OPTION) {
				for (int i = rows.length-1; i >= 0; i--) {
					Mote m = rowToVD(rows[i]);
					VDTableM.removeRow(VDTable.convertRowIndexToModel(rows[i]));
					MoteManager.getInstance().removeMote(m);
				}
			}
		}
		else if ("clearRP".equals(act)) {
			RPTableM.setRowCount(0);
		}
		else if ("removeRP".equals(act)) {
			int[] rows = RPTable.getSelectedRows();
			String message = String.format("ȷ��ɾ��ѡ�е�%d��RP��", rows.length);
			int r = JOptionPane.showConfirmDialog(Main.gui, message, "ȷ��",0,3);
			if (r == JOptionPane.YES_OPTION) {
				for (int i = rows.length-1; i >= 0; i--) {
					Mote m = rowToRP(rows[i]);
					RPTableM.removeRow(RPTable.convertRowIndexToModel(rows[i]));
					MoteManager.getInstance().removeMote(m);
				}
			}
		}
		else if ("clearAP".equals(act)) {
			APTableM.setRowCount(0);
		}
		else if ("removeAP".equals(act)) {
			int[] rows = APTable.getSelectedRows();
			String message = String.format("ȷ��ɾ��ѡ�е�%d��AP��", rows.length);
			int r = JOptionPane.showConfirmDialog(Main.gui, message, "ȷ��",0,3);
			if (r == JOptionPane.YES_OPTION) {
				for (int i = rows.length-1; i >= 0; i--) {
					Mote m = rowToAP(rows[i]);
					APTableM.removeRow(APTable.convertRowIndexToModel(rows[i]));
					MoteManager.getInstance().removeMote(m);
				}
			}
		}
	}

	class VDRadioCellRenderer extends DefaultTableCellRenderer
	{
		final Color closedBg = new Color(240,240,240);

		public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column)
		{
			Component c = super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);
			if (value instanceof String) {
				if (((String)value).equals("�Ѽ���")) {
					setBackground(Color.orange);
					setForeground(Color.black);
				} else if (((String)value).equals("�ѹر�")){
					setBackground(closedBg);
					setForeground(Color.black);
				} else {
					setBackground(null);
					setForeground(Color.black);
				}
			}
			return c;
		}
	}

}
