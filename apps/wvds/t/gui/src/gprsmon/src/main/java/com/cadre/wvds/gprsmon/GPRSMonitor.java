package com.cadre.wvds.gprsmon;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.RowFilter;
import javax.swing.SwingConstants;
import javax.swing.border.BevelBorder;
import javax.swing.border.TitledBorder;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.LineIterator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.util.BytesUtil;
import net.skaldchen.commons.util.JTableUtil;
import net.skaldchen.commons.comm.PacketListener;

public class GPRSMonitor extends JFrame implements PacketListener {

	private static final Logger log = LoggerFactory.getLogger(GPRSMonitor.class);

	private final int W = 900;
	private final int H = 480;

	private DefaultTableModel tableM;
	private JTable table;
	private JLabel statusLabel;
	private JLabel serverLabel;
	private JLabel signalLabel;
	private JLabel gprsCountLabel;
	private JLabel dataCountLabel;

	private final String[] HEADER = new String[]{"时刻","长度","文本","数据","消息","长度","有效载荷"};
	private final int[] WIDTH = new int[]{90,30,150,200,45,30,350};
	private SimpleDateFormat msecFmt = new SimpleDateFormat("HH:mm:ss.SSS");

	private final int STATE_NA      = 0;
	private final int STATE_NOCONN  = 1;
	private final int STATE_CONNING = 2;
	private final int STATE_CLOSED  = 3;
	private final int STATE_CONNOK  = 4;
	private int gprsState = STATE_NA;

	private int statReboot = 0;
	private int statDisconn = 0;
	private int statError = 0;
	private int statUpAck = 0;
	private int statUpNoAck = 0;
	private int statDownAck = 0;
	private int statDownNoAck = 0;

	private TableRowSorter<TableModel> rowSorter;

	private JTextField filterInput;

	private int maxRowCount = 5000;
	private int maxClearInc =  100;

	private boolean in_replay = false;

	private JButton replayLogBtn;

	public GPRSMonitor() {
		setTitle("GPRS分析器/M26");
		setSize(W, H);
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		getContentPane().setLayout(new BorderLayout());

		JPanel panel = new JPanel();
		getContentPane().add(panel, BorderLayout.CENTER);
		panel.setLayout(new BorderLayout());
		panel.setBorder(new TitledBorder("GPRS输出"));

		JPanel north = new JPanel();
		panel.add(north, BorderLayout.NORTH);

		JLabel filterPre = new JLabel("过滤:");
		north.add(filterPre);

		filterInput = new JTextField();
		filterInput.setColumns(20);
		north.add(filterInput);

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

		JButton clearDataBtn = new JButton("清除数据");
		north.add(clearDataBtn);
		clearDataBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				clearTable();
			}
		});

		JButton clearCountBtn = new JButton("清除计数");
		north.add(clearCountBtn);
		clearCountBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				log.info(String.format("clear count at Reboot %d, Closed %d, Error %d,"
						+ " UpAck %d, UpNoAck %d, DownAck %d, DownNoAck %d",
						statReboot, statDisconn, statError,
						statUpAck, statUpNoAck, statDownAck, statDownNoAck));
				clearCount();
			}
		});

		replayLogBtn = new JButton("重放日志");
		north.add(replayLogBtn);
		replayLogBtn.setActionCommand("replay");
		replayLogBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String act = e.getActionCommand();

				if ("replay".equals(act)) {
					clearTable();
					clearCount();
					clearStatus();

					String fname = "./log";
					File file1 = new File(fname);
					JFileChooser chooser = new JFileChooser(file1);
					int returnVal = chooser.showOpenDialog(null);
					if (returnVal == JFileChooser.APPROVE_OPTION) {
						in_replay = true;
						replayLogBtn.setText("退出重放");
						replayLogBtn.setActionCommand("replay_quit");

						final String logfile = chooser.getSelectedFile().getPath();
						Runnable r = new Runnable() {
							@Override
							public void run() {
								try {
									LineIterator it = FileUtils.lineIterator(new File(logfile));
									Pattern p = Pattern.compile("^.* rcvd: \\(([0-9]+)\\) ([0-9A-F ]+)$");
									Matcher m = null;
									while (it.hasNext()) {
										String line = it.next();
										m = p.matcher(line);
										if (m.find()) {
											byte[] pkt = BytesUtil.toBytes(m.group(2));
											addPacket(pkt);
										}
									}
								} catch (IOException e1) {
									log.warn("fail load", e1);
								}
							}
						};
						new Thread(r).start();
					}
				}
				else if ("replay_quit".equals(act)) {
					in_replay = false;
					replayLogBtn.setText("重放日志");
					replayLogBtn.setActionCommand("replay");

					clearTable();
					clearCount();
					clearStatus();
				}
			}
		});

		JScrollPane scrollPane = new JScrollPane();
		panel.add(scrollPane, BorderLayout.CENTER);
		//getContentPane().add(scrollPane, BorderLayout.CENTER);

		tableM = new DefaultTableModel(HEADER, 0);
		table = new JTable(tableM);
		scrollPane.setViewportView(table);
		JTableUtil.fitTableColumns(table, WIDTH);

		rowSorter = new TableRowSorter<TableModel>(tableM);
		table.setRowSorter(rowSorter);

		JPanel panel_1 = new JPanel();
		getContentPane().add(panel_1, BorderLayout.SOUTH);
		panel_1.setPreferredSize(new Dimension(W-10, 28));
		panel_1.setBorder(new BevelBorder(BevelBorder.RAISED));

		statusLabel = new JLabel();
		panel_1.add(statusLabel);
		statusLabel.setPreferredSize(new Dimension(80,18));
		//statusLabel.setOpaque(true);
		//statusLabel.setBackground(Color.yellow);

		serverLabel = new JLabel();
		panel_1.add(serverLabel);
		serverLabel.setPreferredSize(new Dimension(180,18));
		//serverLabel.setOpaque(true);
		//serverLabel.setBackground(Color.red);

		signalLabel = new JLabel();
		panel_1.add(signalLabel);
		signalLabel.setPreferredSize(new Dimension(120,18));
		//signalLabel.setOpaque(true);
		//signalLabel.setBackground(Color.cyan);

		gprsCountLabel = new JLabel();
		panel_1.add(gprsCountLabel);
		gprsCountLabel.setPreferredSize(new Dimension(200,18));
		//gprsCountLabel.setOpaque(true);
		//gprsCountLabel.setBackground(Color.orange);

		dataCountLabel = new JLabel();
		panel_1.add(dataCountLabel);
		dataCountLabel.setHorizontalAlignment(SwingConstants.TRAILING);
		dataCountLabel.setPreferredSize(new Dimension(W-630,18));
		//dataCountLabel.setOpaque(true);
		//dataCountLabel.setBackground(Color.green);

		clearStatus();
		updateStatus();
	}

	private void updateFilter() {
		String text = filterInput.getText().trim();
		if (text != null)
			rowSorter.setRowFilter(RowFilter.regexFilter(text));
	}

	private void clearTable() {
		tableM.setRowCount(0);
	}

	private void clearCount() {
		statReboot = 0;
		statDisconn = 0;
		statError = 0;
		statUpAck = 0;
		statUpNoAck = 0;
		statDownAck = 0;
		statDownNoAck = 0;
		updateStatus();
	}

	private void clearStatus() {
		statusLabel.setText("状态: 未知");
		serverLabel.setText("服务器: ");
		signalLabel.setText("信号: ");
	}

	private String strState(int state) {
		String s = "";
		switch (state) {
		case STATE_NOCONN:
			s = "未连接"; break;
		case STATE_CONNING:
			s = "连接中"; break;
		case STATE_CLOSED:
			s = "被关闭"; break;
		case STATE_CONNOK:
			s = "已连接"; break;
		default:
			s = "未知"; break;
		}
		return s;
	}

	private void updateStatus() {
		statusLabel.setText(String.format("状态: %s", strState(gprsState)));
		gprsCountLabel.setText(String.format("重启: %5d   断开: %5d   错误: %5d",
				statReboot, statDisconn, statError));
		dataCountLabel.setText(String.format("上行\u25B2: %6d / %6d    下行\u25BC: %6d / %6d",
				statUpAck, statUpNoAck, statDownAck, statDownNoAck));
	}

	private byte[] getPayload(byte[] packet) throws Exception {
		final String key = "aeskey-cadre2016";
		final String iv  = "0123456789ABCDEF";

		byte[] out = null;
		if (packet[0] == (byte)0xAA && packet[packet.length-1] == (byte)0xFF) {
			out = WVDS.decrypt(packet, key, iv);
		} else {
			String txt = BytesUtil.txtBytes(packet, 0, packet.length);
			if (txt.matches("^IPD[0-9]+:.*")) {
				int beg = txt.indexOf(":") + 1;
				int len = packet.length;
				byte[] pkt = BytesUtil.getBytes(packet, beg, len-beg);
				out = WVDS.decrypt(pkt, key, iv);
			}
		}
		return out;
	}

	private void addPacket(byte[] packet) {
		try {
			String txt = BytesUtil.txtBytes(packet, 0, packet.length);
			String str = BytesUtil.strBytes(packet);
			byte[] payload = getPayload(packet);

			Object[] rowData = new Object[HEADER.length];
			int i = 0;
			rowData[i++] = msecFmt.format(new Date());
			rowData[i++] = String.valueOf(packet.length);
			rowData[i++] = txt;
			rowData[i++] = str;
			rowData[i++] = (payload != null ? String.format("0x%02X %s", (payload[6] & 0x3F), ((payload[6] & 0x80) > 0 ? "\u25BC" : "\u25B2")) : "");
			rowData[i++] = (payload != null ? String.valueOf(payload.length) : "");
			rowData[i++] = (payload != null ? BytesUtil.strBytes(payload) : "");
			tableM.insertRow(0, rowData);
			//tableM.addRow(rowData);

			if (tableM.getRowCount() >= maxRowCount + maxClearInc) {
				tableM.setRowCount(maxRowCount);
			}

			if (txt.contains("\\nSEND OK\\r") || txt.contains("\\nCONNECT OK\\r")) {
				gprsState = STATE_CONNOK;
			}
			else if (txt.contains("\\nCLOSED\\r")) {
				gprsState = STATE_CLOSED;
				statDisconn++;
			}
			else if (txt.contains("QIOPEN")) {
				gprsState = STATE_CONNING;
				Pattern p = Pattern.compile("^.*\"(TCP|UDP)\",\"([0-9.]+)\",\"([0-9]+)\".*");
				Matcher m = p.matcher(txt);
				if (m.find()) {
					String host = m.group(2);
					String port = m.group(3);
					serverLabel.setText(String.format("服务器: %s:%s", host, port));
				}
			}
			else if (txt.contains("ERROR")) {
				statError++;
			}
			else if (txt.contains("\\nRDY\\r")) {
				gprsState = STATE_NOCONN;
				statReboot++;
			}
			else if (txt.contains("+CSQ:")) {
				Pattern p = Pattern.compile("^.*CSQ: ([0-9]+),([0-9]+).*$");
				Matcher m = p.matcher(txt);
				if (m.find()) {
					int rssiRaw = Integer.parseInt(m.group(1));
					int ber = Integer.parseInt(m.group(2));
					int rssi = -53 + (rssiRaw - 30) * 2;
					signalLabel.setText(String.format("信号: %d,%d/%ddbm", rssiRaw, ber, rssi));
				}
			}

			if (payload != null) {
				boolean up = ((payload[6] & 0x80) == 0);
				boolean ack = ((payload[6] & 0x40) > 0);
				int op = (payload[6] & 0xFF);
				byte[] dev = BytesUtil.getBytes(payload, 0, 6);
				byte[] ts = BytesUtil.getBytes(payload, 7, 6);

				if (!in_replay) log.info(String.format("data: (%d) %s", payload.length, BytesUtil.strBytes(payload)));
				if (up) {
					if (!in_replay) log.info(String.format(" msg: %s from %s %s, ts %s", WVDS.typeName(op), WVDS.NAME[payload[0]], WVDS.strDev(dev), WVDS.strTS(ts)));
					if (ack) {
						statUpAck++;
					} else {
						statUpNoAck++;
					}
				} else {
					if (!in_replay) log.info(String.format(" msg: %s to %s %s, ts %s", WVDS.typeName(op), WVDS.NAME[payload[0]], WVDS.strDev(dev), WVDS.strTS(ts)));
					if (ack) {
						statDownAck++;
					} else {
						statDownNoAck++;
					}
				}
			}
			updateStatus();
		} catch(Exception e) {
			e.printStackTrace();
		}
	}

	@Override
	public void packetReceived(byte[] packet) {
		if (!in_replay) {
			log.info(String.format("rcvd: (%d) %s", packet.length, BytesUtil.strBytes(packet)));
			log.info(String.format("text: %s", BytesUtil.txtBytes(packet, 0, packet.length)));
			addPacket(packet);
		}
	}

}
