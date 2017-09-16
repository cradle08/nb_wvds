package com.cadre.wvds.sniffer;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

import javax.swing.JButton;
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
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

import org.apache.commons.io.FileUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.packet.Packet;
import net.skaldchen.commons.serial.PacketListener;
import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.JTableUtil;

public class TabParkEvent extends JPanel implements PacketListener {

	private static final Logger log = LoggerFactory.getLogger(TabParkEvent.class);

	private String[] HEADER = new String[]{"车位号","节点编号","检测时刻","接收时刻","事件","序号","发送次数","累计次数","备注"};
	private int[] WIDTH = new int[]{60,90,130,130,80,40,55,55,250};
	private DefaultTableModel tableM;
	private JTable table;
	private TableRowSorter<TableModel> rowSorter;
	private JTextField filterInput;

	private final int COL_PARKLOT = 0;
	private final int COL_DEVMAC = 1;
	private final int COL_EVT_TIME = 2;
	private final int COL_RCV_TIME = 3;
	private final int COL_EVENT = 4;
	private final int COL_SEQNO = 5;
	private final int COL_SEND_NUM = 6;
	private final int COL_TOTAL = 7;
	private final int COL_REMARK = 8;

	private final SimpleDateFormat secFmt = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

	private Map<Mote,Map<String,Integer>> evtMap = null;

	public TabParkEvent() {
		setLayout(new BorderLayout());

		JPanel north = new JPanel();
		add(north, BorderLayout.NORTH);

		JLabel filter = new JLabel("过滤:");
		north.add(filter);
		filterInput = new JTextField();
		north.add(filterInput);
		filterInput.setColumns(40);

		filterInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				updateFilter();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				updateFilter();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				updateFilter();
			}
		});

		JButton clearBtn = new JButton("清除");
		north.add(clearBtn);
		clearBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				clear();
			}
		});

		JButton saveBtn = new JButton("保存");
		//north.add(saveBtn);
		saveBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				final SimpleDateFormat fmt = new SimpleDateFormat("yyyyMMdd_HHmmss");
				final String sep = ", ";
				final String eol = "\r\n";
				String fname = String.format("%s.csv", fmt.format(new Date()));

				StringBuilder sb = new StringBuilder();
				for (int i = 0; i < HEADER.length; i++) {
					if (sb.length() > 0) sb.append(sep);
					sb.append(HEADER[i]);
				}
				sb.append(eol);
				for (int i = 0; i < table.getRowCount(); i++) {
					for (int j = 0; j < HEADER.length; j++) {
						if (j > 0) sb.append(sep);
						sb.append((String)table.getValueAt(i, j));
					}
					sb.append(eol);
				}

				try {
					File f = new File(fname);
					FileUtils.writeStringToFile(f, sb.toString(), "cp936");
					String message = String.format("已保存到%s", f.getAbsolutePath());
					JOptionPane.showMessageDialog(TabParkEvent.this, message);
				} catch (IOException e1) {
					log.warn("fail save parking events to " + fname, e1);
				}
			}
		});

		JScrollPane scrollPane = new JScrollPane();
		add(scrollPane, BorderLayout.CENTER);

		tableM = new DefaultTableModel(HEADER, 0);
		table = new JTable(tableM);
		scrollPane.setViewportView(table);
		JTableUtil.fitTableColumns(table, WIDTH);
		table.setRowHeight(20);

		rowSorter = new TableRowSorter<TableModel>(tableM);
		table.setRowSorter(rowSorter);

		TableColumn col = table.getColumnModel().getColumn(COL_EVENT);
		col.setCellRenderer(new VDStateCellRenderer());
	}

	public void clear() {
		tableM.setRowCount(0);
	}

	private void updateFilter() {
		String text = null;
		try {
			text = filterInput.getText().trim();
			if (text != null)
				rowSorter.setRowFilter(RowFilter.regexFilter(text));
		} catch(Exception e) {
			log.warn("fail set filter: " + text, e);
		}
	}

	public void addPacket(Packet p, Date rtime) {
		if (p == null)
			return;

		if ("park evt".equals(p.getName())) {
			byte[] ts = p.getField("ts").value;
			String sTs = BytesUtil.strBytes(ts);
			Calendar cal = Calendar.getInstance();
			cal.set(Calendar.YEAR, 2000 + BytesUtil.getBCD(ts, 0));
			cal.set(Calendar.MONTH, BytesUtil.getBCD(ts, 1) - 1);
			cal.set(Calendar.DAY_OF_MONTH, BytesUtil.getBCD(ts, 2));
			cal.set(Calendar.HOUR_OF_DAY, BytesUtil.getBCD(ts, 3));
			cal.set(Calendar.MINUTE, BytesUtil.getBCD(ts, 4));
			cal.set(Calendar.SECOND, BytesUtil.getBCD(ts, 5));
			int status = p.getField("status").toInt();
			int seqno = p.getField("seqno").toInt();
			byte[] dev = p.getField("dev").value;
			String sDev = WVDS.strDev(dev);
			byte[] mac = new byte[8];
			System.arraycopy(dev, 0, mac, 2, 6);

			MoteManager motedb = MoteManager.getInstance();
			Mote m = motedb.getMoteByMAC(WVDS.dev2mac(dev));
			if (m == null) {
				m = new Mote();
				m.setMac(mac);
				m.setDev(dev);
				m.setName("N/A");
				motedb.addMote(m);
				motedb.save();
			}

			Map<String,Integer> map = null;
			if (evtMap == null) {
				evtMap = new HashMap<Mote,Map<String,Integer>>();
			}
			map = evtMap.get(m);
			if (map == null) {
				map = new HashMap<String,Integer>();
				evtMap.put(m, map);
			}
			if (map.get(sTs) == null) {
				map.put(sTs, 0);
			}
			int num = map.get(sTs);
			num += 1;
			map.put(sTs, num);

			if (num == 1) {
				m.eventN += 1;
				Object[] row = new Object[HEADER.length];
				int i = 0;
				row[i++] = m.getName(); // 泊位号
				row[i++] = sDev; // 节点编号
				row[i++] = secFmt.format(cal.getTime()); // 发生时刻
				row[i++] = secFmt.format(rtime); // 接收时刻
				row[i++] = WVDS.strState(status); // 事件
				row[i++] = Integer.valueOf(seqno); // 序号
				row[i++] = Integer.valueOf(num); // 发送
				row[i++] = (m != null ? String.valueOf(m.eventN) : "0"); // 累计次数
				row[i++] = ""; // 备注
				tableM.insertRow(0, row);
				//tableM.addRow(row);
			} else {
				for (int j = 0; j < tableM.getRowCount(); j++) {
					if (sDev.equals((String)table.getValueAt(j, 1))) {
						if (seqno == (Integer)table.getValueAt(j, 5)) {
							table.setValueAt(Integer.valueOf(num), j, 6);
						}
					}
				}
			}
		}
	}

	public void addPacket(byte[] packet, Date rtime) {
		try {
			Packet p = Main.parser.parse(packet);
			addPacket(p, rtime);
		} catch (Exception e) {
			log.warn(String.format("fail add packet: %s, %s", secFmt.format(rtime), BytesUtil.strBytes(packet)), e);
		}
	}

	@Override
	public void packetReceived(byte[] packet) {
		addPacket(packet, new Date());
	}

}
