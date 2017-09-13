package com.cadre.wvds.sniffer;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Insets;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

import javax.swing.BorderFactory;
import javax.swing.ButtonGroup;
import javax.swing.ComboBoxModel;
import javax.swing.DefaultComboBoxModel;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JRadioButtonMenuItem;
import javax.swing.JScrollBar;
import javax.swing.JScrollPane;
import javax.swing.JSeparator;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;
import javax.swing.JTable;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.JToolBar;
import javax.swing.KeyStroke;
import javax.swing.ListSelectionModel;
import javax.swing.RowFilter;
import javax.swing.SwingUtilities;
import javax.swing.Timer;
import javax.swing.border.BevelBorder;
import javax.swing.border.TitledBorder;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

import org.apache.commons.io.FileUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.alibaba.fastjson.JSON;

import net.skaldchen.commons.packet.Packet;
import net.skaldchen.commons.serial.CommAdapter;
import net.skaldchen.commons.serial.PacketListener;
import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.I18N;
import net.skaldchen.commons.utils.JTableUtil;
import net.skaldchen.commons.utils.Uptime;

/**
 * This code was edited or generated using CloudGarden's Jigloo
 * SWT/Swing GUI Builder, which is free for non-commercial
 * use. If Jigloo is being used commercially (ie, by a corporation,
 * company or business for any purpose whatever) then you
 * should purchase a license for each developer using Jigloo.
 * Please visit www.cloudgarden.com for details.
 * Use of Jigloo implies acceptance of these licensing terms.
 * A COMMERCIAL LICENSE HAS NOT BEEN PURCHASED FOR
 * THIS MACHINE, SO JIGLOO OR THIS CODE CANNOT BE USED
 * LEGALLY FOR ANY CORPORATE OR COMMERCIAL PURPOSE.
 */
public class SnifferApp extends javax.swing.JFrame implements PacketListener, ActionListener {

	private static final boolean new_at_top = true;
	private static final String[] DELUGE_CMD = { "", "stop", "local stop", "only diss", "diss reprog", "reprog", "reboot" };

	private String firmwareVer = null;
	private String firmwareCommit = null;
	private String firmwareBuild  = null;

	private JMenuItem helpMenuItem;
	private JMenuItem aboutMenuItem;
	private JRadioButtonMenuItem debugMenuItem;
	private JMenu jMenu5;
	private JScrollPane descriptionTableScrollPane;
	private JScrollPane messageTableScrollPane;
	//private JTextArea outputArea;
	private DefaultTableModel outputTableM;
	private JTable outputTable;
	private JScrollPane outputScrollPane;
	private JTable messageTable;
	private JTable descriptionTable;
	private JPanel jPanel2;
	public JSplitPane jPanel4;
	public JSplitPane jPanel1;
	private JMenuItem deleteMenuItem;
	private JMenuItem editMsgsMenuItem;
	private JMenuItem editAlgoMenuItem;
	private JSeparator jSeparator1;
	private JMenuItem pasteMenuItem;
	private JMenuItem copyMenuItem;
	private JMenuItem cutMenuItem;
	private JMenu jMenu4;
	private JMenuItem exitMenuItem;
	private JSeparator jSeparator2;
	private JMenuItem closeFileMenuItem;
	private JMenuItem saveAsMenuItem;
	private JMenuItem saveMenuItem;
	private JButton prevButton;
	private JButton nextButton;
	private JTextField findTextField;
	private JLabel findLabel;
	private JPanel jPanel5;
	private JPanel jPanel6;
	private JComboBox<String> packetCombo;
	private JTextField packetEdit;
	private JPanel jPanel3;
	private JMenuItem openFileMenuItem;
	private JMenuItem newFileMenuItem;
	private JMenu jMenu3;
	private JMenuBar jMenuBar1;
	private DefaultTableModel descriptionTableModel;
	private DefaultTableModel messageTableModel;
	private JLabel hintLabel;
	private JLabel countLabel;
	private JLabel sepLabel4;
	private JTextField filterTextField;
	private JButton clearBtn;
	private JLabel filterLabel;
	private JLabel sepLabel2;
	private JLabel chanLabel;
	private JComboBox channelCombo;
	private ActionListener channelListener;
	private JLabel txpowerLabel;
	private JComboBox txpowerCombo;
	private ActionListener txpowerListener;
	private JComboBox groupComboBox;
	private JLabel groupLabel;
	private TableRowSorter<TableModel> descriptionRowSorter;
	private JMenuItem clearMenuItem;
	private JMenuItem reloadMenuItem;
	private ActionListener menuListener;
	private JTabbedPane jTabbedPane1;
	private JPanel tabPacket;
	private JPanel tabTopology;
	//    private NetworkMap networkMap;
	private TabMagnetic tabMagnet;
	private TabParkEvent tabParkEvent;
	public TabAnalyse tabAnalyse;
	private JPanel mapControl;
	private JScrollBar mapHoriScroll;
	private JScrollBar mapVertScroll;
	private JComboBox mapModeCombo;
	private JButton mapClearBtn;
	private JButton beaconBtn;
	public static JCheckBox mapCircleCheck;
	public static JCheckBox moteTextCheck;
	public static JCheckBox inqualityCheck;
	private JTextField moteFilterInput;
	private JButton moteRecruitBtn;

	private JMenu jMenu6;
	private JRadioButtonMenuItem[] appMenuItems;
	private final String[] APPs = {"wsnsems", "default"};
	//private EnvRssiPane tabEnvRssi;

	private JToolBar toolBar;
	private JButton[] toolBarButtons;
	private Timer statusT;

	private int WIDTH  = 960;
	private int HEIGHT = 640;
	public static boolean DEBUG = false;

	private MoteManager motemgr = null;
	private CommAdapter comm = null;
	private File dataFile = null;

	private String[] descriptionColumns = new String[] { I18N.get("col.Line"), I18N.get("col.Time"), I18N.get("col.RSSI"), I18N.get("col.Chan"), I18N.get("col.PAN"), I18N.get("col.Name"), I18N.get("col.ID"), I18N.get("col.Type"), I18N.get("col.Description") };
	private int[] descriptionColumnWidths = new int[] { 50,120,40,40,30,70,50,100,WIDTH-510 };

	private int rcvdCount = 0;

	private int activeChan = 6;
	private int activePower = 17;

	private Uptime uptime;
	private Timer secTimer;

	public SnifferApp() {
		super();
		initGUI();
		init();
		log.info("------ sniffer restart ------");
	}

	public void setComm(CommAdapter comm) {
		this.comm = comm;
	}

	private void init() {
		statusT = new Timer(2000, new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				hintLabel.setText("");
			}
		});
		motemgr = MoteManager.getInstance();

		SimpleDateFormat fmt = new SimpleDateFormat("yyyyMMdd_HHmmss");
		dataFile = new File(String.format("%s/parking-%s.log",
				Config.get("data-dir"), fmt.format(new Date())));

		packetCombo.removeAllItems();
		packetCombo.addItem("SetPeriod");

		packetCombo.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String typ = (String) packetCombo.getSelectedItem();
				if (typ.equals("SetPeriod")) {
					String msg = "Input sample period:";
					String input = JOptionPane.showInputDialog(msg);
					if (input != null && input.matches("^[0-9]+$")) {
						int period = Integer.parseInt(input);
						String cmd = String.format("AT+setPeriod %d\r\n", period);
						packetEdit.setText(BytesUtil.strBytes(cmd.getBytes()));
					}
				}
			}
		});
	}

	private void exit() {
		boolean quit = true;
		if (Config.getBool("quit-confirm")) {
			String message = "确定退出Sniffer程序吗?";
			int r = JOptionPane.showConfirmDialog(this, message, "确认",0,3);
			quit = (r == JOptionPane.YES_OPTION);
		}
		if (quit) {
			Point p = this.getLocation();
			Config.set("frame-x", String.valueOf(p.x));
			Config.set("frame-y", String.valueOf(p.y));
			Config.save();
			FileUtils.deleteQuietly(new File(Main.pidfile));
			this.dispose();
			System.exit(0);
		}
	}

	private void initGUI() {
		menuListener = new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				JMenuItem item = (JMenuItem) e.getSource();
				String action = e.getActionCommand();
				//debug("clicked menu action " + action);

				if (action.equals("Clear")) {
					clear();
				}
				else if (action.equals("reload")) {
					Main.parser.reload();
					showStatusMsg(I18N.get("msg.reloadok"));
				}
				else if (action.equals("Exit")) {
					exit();
				}
				else if (action.equals("Debug")) {
					DEBUG = debugMenuItem.isSelected();
				}
				else if (action.equals("Save")) {
					save();
				}
				else if (action.equals("EditMsgs")) {
					editMsgsXml();
				}
				else if (action.equals("EditAlgo")) {
					showAlgoDialog();
				}
				else if (action.startsWith("app.")) {
					String app = action.substring(4);
					Config.set("sniffer-app", app, true);
				}
				else if (action.equals("Help")) {
					try {
						String cmd = "cmd /c start %s";
						cmd = String.format(cmd, Config.get("help-file"));
						Runtime.getRuntime().exec(cmd);
					} catch(Exception ex) {
						ex.printStackTrace();
					}
				}
				else if (action.equals("About")) {
					Runnable runnable = new Runnable() {
						public void run() {
							getFirmwareVer();

							String aboutText = I18N.get("text.about")
									.replaceAll("<version>", Version.app_version.toString())
									.replaceAll("<build>", Version.build)
									.replaceAll("<commit>", Version.commit)
									.replaceAll("<fwver>", (firmwareVer != null ? firmwareVer : "N/A"))
									.replaceAll("<fwcommit>", (firmwareCommit != null ? firmwareCommit : "N/A"))
									.replaceAll("<fwbuild>",  (firmwareBuild != null ? firmwareBuild : "N/A"));
							JOptionPane.showMessageDialog(Main.gui,
									aboutText, I18N.get("title.about"),
									JOptionPane.INFORMATION_MESSAGE);
						}
					};
					SwingUtilities.invokeLater(runnable);
				}
			}
		};

		try {
			WIDTH = Config.getInt("frame-width");
			HEIGHT = Config.getInt("frame-height");
			setSize(WIDTH, HEIGHT);
			setTitle(I18N.get("title.app"));
			setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);

			addWindowListener(new WindowAdapter() {
				@Override
				public void windowClosing(WindowEvent e) {
					exit();
				}
			});

			getContentPane().setLayout(new BorderLayout());
			// -------- toolbar --------
			createToolBar();
			getContentPane().add(toolBar, BorderLayout.NORTH);

			// -------- main -------
			{
				jTabbedPane1 = new JTabbedPane();
				{
					tabPacket = new JPanel();
					tabPacket.setLayout(new BorderLayout());
				}
				{
					tabTopology = new JPanel();
					tabTopology.setLayout(new BorderLayout());
				}
				{
					//tabEnvRssi = new EnvRssiPane();
				}
				{
					tabMagnet = new TabMagnetic();
					MoteManager.getInstance().addView(tabMagnet);
				}
				{
					tabParkEvent = new TabParkEvent();
					//Main.comm.addPacketListener(tabParkEvent);
				}
				{
					tabAnalyse = new TabAnalyse();
					//Main.comm.addPacketListener(tabAnalyse);
				}
				jTabbedPane1.addTab(I18N.get("tab.Packet"), tabPacket);
				jTabbedPane1.addTab(I18N.get("tab.Analyse"), tabAnalyse);
				jTabbedPane1.addTab(I18N.get("tab.ParkEvent"), tabParkEvent);
				jTabbedPane1.addTab(I18N.get("tab.Magnetic"), tabMagnet);
				//jTabbedPane1.addTab("Topology", tabTopology);
				//jTabbedPane1.addTab("Env RSSI", tabEnvRssi);
				getContentPane().add(jTabbedPane1, BorderLayout.CENTER);
			}
			{
				jPanel1 = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
				tabPacket.add(jPanel1, BorderLayout.CENTER);
				{
					descriptionTableScrollPane = new JScrollPane();
					jPanel1.add(descriptionTableScrollPane, JSplitPane.TOP);
					descriptionTableScrollPane.setBorder(BorderFactory.createTitledBorder(null, I18N.get("table.Description"), TitledBorder.LEADING, TitledBorder.DEFAULT_POSITION));
					{
						descriptionTableModel =
								new DefaultTableModel(
										new String[][] { { "", "", "", "", "", "", "", "", "" } },
										descriptionColumns);
						descriptionTable = new JTable();
						descriptionTableScrollPane.setViewportView(descriptionTable);
						descriptionTable.setModel(descriptionTableModel);
						descriptionTableModel.setRowCount(0);

						// only the last column resizable
						//for (int i = 0; i < descriptionTable.getColumnCount()-1; i++)
						//    descriptionTable.getColumnModel().getColumn(i).setResizable(false);
						//descriptionTable.setAutoResizeMode(JTable.AUTO_RESIZE_LAST_COLUMN);

						descriptionTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
						descriptionTable.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
							public void valueChanged(ListSelectionEvent e) {
								if (!e.getValueIsAdjusting()) {
									int[] rows = descriptionTable.getSelectedRows();
									if (rows.length > 0) {
										int row = Integer.parseInt((String)descriptionTable.getValueAt(rows[0], 0));
										debug("highlight row " + row);
										if (new_at_top) {
											int rmin = 0, rmax = messageTable.getRowCount()-1;
											int r = 0;
											int crow = 0;
											while (rmin < rmax) {
												debug("  checking [" + rmin + ", " + rmax + "]");
												if ((rmax + rmin) % 2 == 0) {
													r = (rmin + rmax) / 2;
													crow = Integer.parseInt((String)messageTable.getValueAt(r, 0));
													if (row == crow) {
														break;
													} else if (row < crow) {
														rmin = r;
													} else {
														rmax = r;
													}
												} else {
													r = (rmin + rmax) / 2;
													crow = Integer.parseInt((String)messageTable.getValueAt(r, 0));
													if (row == crow) {
														break;
													} else if (row > crow) {
														rmax = r;
													} else {
														r = (rmin + rmax + 1) / 2;
														crow = Integer.parseInt((String)messageTable.getValueAt(r, 0));
														if (row == crow) {
															break;
														} else if (row < crow) {
															rmin = r;
														}
													}
												}
											}
											debug("  found row " + r);
											messageTable.getSelectionModel().setSelectionInterval(r, r);
											Rectangle cellLocation = messageTable.getCellRect(r, 0, false);
											messageTableScrollPane.getVerticalScrollBar().setValue(cellLocation.y);
										} else {
											messageTable.getSelectionModel().setSelectionInterval(row, row);
											Rectangle cellLocation = messageTable.getCellRect(row, 0, false);
											messageTableScrollPane.getVerticalScrollBar().setValue(cellLocation.y);
										}
									}
								}
							}
						});

						JTableUtil.fitTableColumns(descriptionTable, descriptionColumnWidths);

						descriptionRowSorter = new TableRowSorter<TableModel>(descriptionTableModel);
						descriptionTable.setRowSorter(descriptionRowSorter);
					}
				}
				{
					jPanel4 = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
					jPanel1.add(jPanel4, JSplitPane.BOTTOM);
					{
						messageTableScrollPane = new JScrollPane();
						jPanel4.add(messageTableScrollPane, JSplitPane.LEFT);
						messageTableScrollPane.setBorder(BorderFactory.createTitledBorder(I18N.get("table.Message")));
						{
							messageTableModel =
									new DefaultTableModel(
											new String[][] { { "", "", "" } },
											new String[] { I18N.get("col.Line"), I18N.get("col.Length"), I18N.get("col.Packet") });
							messageTable = new JTable();
							messageTableScrollPane.setViewportView(messageTable);
							messageTable.setModel(messageTableModel);
							messageTableModel.setRowCount(0);

							// only the last column resizable
							//for (int i = 0; i < messageTable.getColumnCount()-1; i++)
							//    messageTable.getColumnModel().getColumn(i).setResizable(false);
							messageTable.setAutoResizeMode(JTable.AUTO_RESIZE_LAST_COLUMN);

							messageTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
							messageTable.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
								public void valueChanged(ListSelectionEvent e) {
									/*if (!e.getValueIsAdjusting()) {
                                        int[] rows = messageTable.getSelectedRows();
                                        if (rows.length > 0) {
                                            int row = rows[0];
                                            //debug("messageTable: selected row " + row);

                                            descriptionTable.getSelectionModel().setSelectionInterval(row, row);
                                            Rectangle cellLocation = descriptionTable.getCellRect(row, 0, false);
                                            descriptionTableScrollPane.getVerticalScrollBar().setValue(cellLocation.y);
                                        }
                                    }*/
								}
							});

							int[] colw = new int[]{60,40,700};
							JTableUtil.fitTableColumns(messageTable, colw);
						}
					}
					{
						//outputArea = new JTextArea();
						//outputArea.setFont(Config.getFont("output-font"));

						String[] HEADER = new String[]{I18N.get("col.Time"),I18N.get("col.Logging")};
						outputTableM = new DefaultTableModel(HEADER, 0);
						outputTable = new JTable(outputTableM);
						outputTable.setShowGrid(false);
						outputTable.setIntercellSpacing(new Dimension(0, 0));

						//outputScrollPane = new JScrollPane(outputArea);
						outputScrollPane = new JScrollPane(outputTable);
						outputScrollPane.setBorder(BorderFactory.createTitledBorder(I18N.get("table.Logging")));
						jPanel4.add(outputScrollPane, JSplitPane.RIGHT);

						Dimension pref = outputTable.getPreferredSize();
						int[] WIDTH = new int[]{60,0};
						WIDTH[1] = (int) (pref.getWidth() - WIDTH[0]);
						JTableUtil.fitTableColumns(outputTable, WIDTH);
					}
				}
			}
			{
				jPanel5 = new JPanel();
				tabPacket.add(jPanel5, BorderLayout.NORTH);
				jPanel5.setLayout(new BorderLayout());
			}
			{
				jPanel3 = new JPanel();
				FlowLayout jPanel3Layout = new FlowLayout();
				jPanel5.add(jPanel3, BorderLayout.NORTH);
				jPanel3.setLayout(jPanel3Layout);
				jPanel3.add(getChanLabel());
				jPanel3.add(getChanCombo());
				jPanel3.add(getTXPowerLabel());
				jPanel3.add(getTXPowerCombo());
				jPanel3.add(getSepLabel(20));
				jPanel3.add(getGroupLabel());
				jPanel3.add(getGroupCombo());
				jPanel3.add(getSepLabel(5));
				jPanel3.add(getMoteLabel());
				//jPanel3.add(getMoteCombo());
				jPanel3.add(getMoteFilter());
				//jPanel3.add(getSepLabel(5));
				//jPanel3.add(getTargetLabel());
				//jPanel3.add(getTargetEdit());
				//jPanel3.add(getResetBtn());
				//jPanel3.add(getFindBtn());
				//jPanel3.add(getCheckBtn());
				jPanel3.add(getSepLabel(30));
				jPanel3.add(getJLabel3());
				jPanel3.add(getTableFilter());
				jPanel3.add(getSepLabel(30));
				jPanel3.add(getClearBtn());
				//jPanel3.add(getSepLabel(5));
				//{
				//    findLabel = new JLabel();
				//    jPanel3.add(findLabel);
				//    findLabel.setText("Find:");
				//}
				//{
				//    findTextField = new JTextField();
				//    jPanel3.add(findTextField);
				//    findTextField.setColumns(8);
				//    findTextField.setText("");
				//}
				//{
				//    nextButton = new JButton();
				//    jPanel3.add(nextButton);
				//    nextButton.setText("Next");
				//    nextButton.setMargin(new Insets(1,1,1,1));
				//    nextButton.setPreferredSize(new Dimension(40,22));
				//    nextButton.addActionListener(new ActionListener() {
				//        @Override
				//        public void actionPerformed(ActionEvent arg0) {
				//            String patt = findTextField.getText().trim();
				//            jumpToMatchRow(patt, FORWARD); // forward
				//        }
				//    });
				//}
				//{
				//    prevButton = new JButton();
				//    jPanel3.add(prevButton);
				//    prevButton.setText("Prev");
				//    prevButton.setMargin(new Insets(1,1,1,1));
				//    prevButton.setPreferredSize(new Dimension(40,22));
				//    prevButton.addActionListener(new ActionListener() {
				//        @Override
				//        public void actionPerformed(ActionEvent arg0) {
				//            String patt = findTextField.getText().trim();
				//            jumpToMatchRow(patt, BACKWARD); // backward
				//        }
				//    });
				//}
			}
			{
				jPanel6 = new JPanel();
				{
					JLabel packetPre = new JLabel();
					packetPre.setText(I18N.get("textf.Packet"));
					jPanel6.add(packetPre);
				}
				{
					packetCombo = new JComboBox();
					jPanel6.add(packetCombo);
					packetCombo.setPreferredSize(new Dimension(108,20));
				}
				{
					packetEdit = new JTextField();
					packetEdit.setColumns(100);
					jPanel6.add(packetEdit);
				}
				{
					JButton sendBtn = new JButton();
					sendBtn.setText(I18N.get("button.Send"));
					sendBtn.setActionCommand("sendPacket");
					sendBtn.addActionListener(this);
					jPanel6.add(sendBtn);
				}
				//jPanel5.add(jPanel6, BorderLayout.CENTER);
			}
			{
				{
					//                    networkMap = new NetworkMap(nwk);
					//                    tabTopology.add(networkMap, BorderLayout.CENTER);
				}
				{
					mapControl = new JPanel();
					tabTopology.add(mapControl, BorderLayout.NORTH);

					ComboBoxModel jComboBox3Model =    new DefaultComboBoxModel(
							new String[] { "LQI", "RSSI" });
					mapModeCombo = new JComboBox();
					mapModeCombo.setModel(jComboBox3Model);
					//mapModeCombo.setEnabled(false);
					mapControl.add(mapModeCombo);
					mapModeCombo.addActionListener(new ActionListener() {
						@Override
						public void actionPerformed(ActionEvent e) {
							JComboBox combo = (JComboBox) e.getSource();
							//                            networkMap.setMode((String) combo.getSelectedItem());
						}
					});

					moteRecruitBtn = new JButton();
					moteRecruitBtn.setText("Recruit");
					mapControl.add(moteRecruitBtn);
					moteRecruitBtn.addActionListener(new ActionListener() {
						@Override
						public void actionPerformed(ActionEvent e) {
							//                            nwk.sendBaseCmd(0xffff, Const.BASE_CMD_QUERYIDENT);
						}
					});

					mapClearBtn = new JButton();
					mapClearBtn.setText("Clear");
					mapControl.add(mapClearBtn);
					mapClearBtn.addActionListener(new ActionListener() {
						@Override
						public void actionPerformed(ActionEvent arg0) {
							//                            networkMap.clear();
						}
					});

					mapCircleCheck = new JCheckBox();
					mapCircleCheck.setText("show Circle");
					mapCircleCheck.setSelected(true);
					mapControl.add(mapCircleCheck);
					mapCircleCheck.addActionListener(new ActionListener() {
						@Override
						public void actionPerformed(ActionEvent arg0) {
							//                            networkMap.setShowCircle(mapCircleCheck.isSelected());
						}
					});

					moteTextCheck = new JCheckBox();
					moteTextCheck.setText("show Text");
					moteTextCheck.setSelected(true);
					mapControl.add(moteTextCheck);
					moteTextCheck.addActionListener(new ActionListener() {
						@Override
						public void actionPerformed(ActionEvent arg0) {
							//                            networkMap.setShowText(moteTextCheck.isSelected());
						}
					});

					inqualityCheck = new JCheckBox();
					inqualityCheck.setText("in Quality");
					inqualityCheck.setSelected(true);
					mapControl.add(inqualityCheck);
					inqualityCheck.addActionListener(new ActionListener() {
						@Override
						public void actionPerformed(ActionEvent arg0) {
						}
					});
				}
				{
					mapHoriScroll = new JScrollBar(JScrollBar.HORIZONTAL);
					tabTopology.add(mapHoriScroll, BorderLayout.SOUTH);
				}
				{
					mapVertScroll = new JScrollBar(JScrollBar.VERTICAL);
					tabTopology.add(mapVertScroll, BorderLayout.EAST);
				}
			}
			{
				jMenuBar1 = new JMenuBar();
				setJMenuBar(jMenuBar1);
				{
					jMenu3 = new JMenu();
					jMenuBar1.add(jMenu3);
					jMenu3.setText(I18N.get("menu.File"));
					{
						newFileMenuItem = new JMenuItem();
						//jMenu3.add(newFileMenuItem);
						newFileMenuItem.setText("New");
					}
					{
						openFileMenuItem = new JMenuItem();
						//jMenu3.add(openFileMenuItem);
						openFileMenuItem.setText("Open");
					}
					{
						saveMenuItem = new JMenuItem();
						jMenu3.add(saveMenuItem);
						saveMenuItem.setText(I18N.get("menu.Save"));
						//saveMenuItem.setAccelerator(KeyStroke.getKeyStroke("ctrl S"));
						saveMenuItem.addActionListener(menuListener);
					}
					{
						saveAsMenuItem = new JMenuItem();
						//jMenu3.add(saveAsMenuItem);
						saveAsMenuItem.setText("Save As ...");
					}
					{
						closeFileMenuItem = new JMenuItem();
						//jMenu3.add(closeFileMenuItem);
						closeFileMenuItem.setText("Close");
					}
					{
						jSeparator2 = new JSeparator();
						jMenu3.add(jSeparator2);
					}
					{
						exitMenuItem = new JMenuItem();
						jMenu3.add(exitMenuItem);
						exitMenuItem.setText(I18N.get("menu.Exit"));
						//exitMenuItem.setAccelerator(KeyStroke.getKeyStroke("ctrl X"));
						exitMenuItem.addActionListener(menuListener);
					}
				}
				{
					jMenu4 = new JMenu();
					jMenuBar1.add(jMenu4);
					jMenu4.setText(I18N.get("menu.Edit"));
					{
						cutMenuItem = new JMenuItem();
						//jMenu4.add(cutMenuItem);
						cutMenuItem.setText("Cut");
					}
					{
						copyMenuItem = new JMenuItem();
						//jMenu4.add(copyMenuItem);
						copyMenuItem.setText("Copy");
					}
					{
						pasteMenuItem = new JMenuItem();
						//jMenu4.add(pasteMenuItem);
						pasteMenuItem.setText("Paste");
					}
					{
						jSeparator1 = new JSeparator();
						jMenu4.add(jSeparator1);
					}
					{
						deleteMenuItem = new JMenuItem();
						//jMenu4.add(deleteMenuItem);
						deleteMenuItem.setText("Delete");
					}
					{
						editMsgsMenuItem = new JMenuItem();
						jMenu4.add(editMsgsMenuItem);
						editMsgsMenuItem.setText("消息定义");
						editMsgsMenuItem.setActionCommand("EditMsgs");
						editMsgsMenuItem.addActionListener(menuListener);
					}
					{
						editAlgoMenuItem = new JMenuItem();
						jMenu4.add(editAlgoMenuItem);
						editAlgoMenuItem.setText("算法参数");
						editAlgoMenuItem.setActionCommand("EditAlgo");
						editAlgoMenuItem.addActionListener(menuListener);
					}
					{
						jMenu4.add(new JSeparator());
					}
					{
						clearMenuItem = new JMenuItem();
						jMenu4.add(clearMenuItem);
						clearMenuItem.setText(I18N.get("menu.Clear"));
						//clearMenuItem.setAccelerator(KeyStroke.getKeyStroke("ctrl C"));
						clearMenuItem.addActionListener(menuListener);
					}
					{
						reloadMenuItem = new JMenuItem();
						jMenu4.add(reloadMenuItem);
						reloadMenuItem.setText(I18N.get("menu.Reload"));
						reloadMenuItem.setActionCommand("reload");
						reloadMenuItem.addActionListener(menuListener);
					}
				}
				{
					jMenu6 = new JMenu();
					//jMenuBar1.add(jMenu6);
					jMenu6.setText("App");
					{
						appMenuItems = new JRadioButtonMenuItem[APPs.length];
						ButtonGroup g = new ButtonGroup();
						for (int i = 0; i < APPs.length; i++) {
							appMenuItems[i] = new JRadioButtonMenuItem();
							jMenu6.add(appMenuItems[i]);
							appMenuItems[i].setText(APPs[i]);
							appMenuItems[i].setActionCommand("app." + APPs[i]);
							appMenuItems[i].addActionListener(menuListener);
							g.add(appMenuItems[i]);
						}

						//for (int i = 0; i < APPs.length; i++)
						//    if (Config.get("sniffer-app").equals(APPs[i]))
						//        appMenuItems[i].setSelected(true);
					}
				}
				{
					jMenu5 = new JMenu();
					jMenuBar1.add(jMenu5);
					jMenu5.setText(I18N.get("menu.Help"));
					{
						helpMenuItem = new JMenuItem();
						jMenu5.add(helpMenuItem);
						helpMenuItem.setText(I18N.get("menu.Help"));
						helpMenuItem.setActionCommand("Help");
						helpMenuItem.addActionListener(menuListener);
					}
					{
						debugMenuItem = new JRadioButtonMenuItem();
						//jMenu5.add(debugMenuItem);
						debugMenuItem.setText("Debug");
						debugMenuItem.setSelected(false);
						debugMenuItem.addActionListener(menuListener);
					}
					jMenu5.add(new JSeparator());
					{
						aboutMenuItem = new JMenuItem();
						jMenu5.add(aboutMenuItem);
						aboutMenuItem.setText(I18N.get("menu.About"));
						aboutMenuItem.setActionCommand("About");
						aboutMenuItem.addActionListener(menuListener);
					}
				}
			}
			{
				jPanel2 = new JPanel();
				BorderLayout jPanel2Layout = new BorderLayout();
				jPanel2.setLayout(jPanel2Layout);
				getContentPane().add(jPanel2, BorderLayout.SOUTH);
				jPanel2.setBorder(BorderFactory.createRaisedBevelBorder());
				{
					hintLabel = new JLabel();
					jPanel2.add(hintLabel, BorderLayout.CENTER);
					hintLabel.setText("");
				}
				{
					countLabel = new JLabel();
					jPanel2.add(countLabel, BorderLayout.EAST);
					countLabel.setPreferredSize(new Dimension(210, 20));
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}

		uptime = new Uptime();
		secTimer = new Timer(1000, new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				uptime.inc();
				updateStatus();
			}
		});
		secTimer.start();
		updateStatus();
	}

	private void updateStatus() {
		countLabel.setText(String.format("%s: %-6d  %s: %s",
				I18N.get("status.rcvd"), rcvdCount, I18N.get("status.uptime"), uptime.toString()));
	}

	private JTextField targetEdit;

	private JTextField getTargetEdit() {
		targetEdit = new JTextField();
		targetEdit.setColumns(6);
		return targetEdit;
	}

	private JLabel getTargetLabel() {
		JLabel label = new JLabel();
		label.setText("Target");
		label.setPreferredSize(new Dimension(40,16));
		return label;
	}

	private JButton getResetBtn() {
		JButton button = new JButton();
		button.setText("R");
		//button.setIcon(new ImageIcon("reset.png"));
		button.setMargin(new Insets(1,1,1,1));
		button.setPreferredSize(new Dimension(22,22));
		button.setToolTipText("Reset the target mote");
		button.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String sid = targetEdit.getText().trim();
				if (sid.matches("^[0-9]+$")) {
					int nid = Integer.parseInt(sid);
					//Main.nwk.resetMote(nid);
					showStatusMsg("reset mote " + nid);
				}
			}
		});
		return button;
	}

	private JButton getFindBtn() {
		JButton button = new JButton();
		button.setText("F");
		//button.setIcon(new ImageIcon("reset.png"));
		button.setMargin(new Insets(1,1,1,1));
		button.setPreferredSize(new Dimension(22,22));
		button.setToolTipText("Find out which channel the target mote stays and switch to it");
		button.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String sid = targetEdit.getText().trim();
				if (sid.matches("^[0-9]+$")) {
					int nid = Integer.parseInt(sid);
					byte num = (byte) Config.getInt("channel-find-round");

					byte[] arg = new byte[]{ num }; // run num rounds
					//Main.nwk.sendSnifferCmd(nid, Const.SNIFFER_CMD_SEARCH, arg);
					showStatusMsg("find mote " + nid + " on which channel");
				}
			}
		});
		return button;
	}

	private JButton getCheckBtn() {
		JButton button = new JButton();
		button.setText("C");
		//button.setIcon(new ImageIcon("reset.png"));
		button.setMargin(new Insets(1,1,1,1));
		button.setPreferredSize(new Dimension(22,22));
		button.setToolTipText("Check the success ratio of radio communication from sniffer to the target");
		button.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String sid = targetEdit.getText().trim();
				if (sid.matches("^[0-9]+$")) {
					int nid = Integer.parseInt(sid);
					byte num = (byte) Config.getInt("testrx-count");

					byte[] arg = new byte[]{ num };
					//Main.nwk.sendSnifferCmd(nid, Const.SNIFFER_CMD_TESTRX, arg);
					showStatusMsg("check whether mote " + nid + " is reachable");
				}
			}
		});
		return button;
	}

	//public void setNetwork(MoteNetwork nwk) {
	//    this.nwk = nwk;
	//    networkMap.setNetwork(nwk);
	//}

	// tool bar item:
	// {label, icon, listener, enable, command, tooltip}
	private Object[][] toolBarData = {
			{ "start", "start.png", this, Boolean.FALSE, "start", null },
			{ "stop", "stop.png", this, Boolean.FALSE, "stop", null },
			{ null },
			{ "edit", "edit.png", this, Boolean.TRUE, "edit", "编辑消息定义文件" },
			{ "reload", "reload.png", this, Boolean.TRUE, "reload", "重新加载消息定义文件" },
			{ null },
			//{ "setting", "setting.png", this, Boolean.TRUE, "setting", null },
			{ "clear", "clear.png", this, Boolean.TRUE, "clear", "清除各标签页的当前数据" },
			{ "replay", "replay.png", this, Boolean.TRUE, "replay", "选择某个日志文件进行数据回放" },
			//{ "select", "select.png", this, Boolean.TRUE, "select", null },
			{ null },
			{ "manager", "manager.png", this, Boolean.TRUE, "manager", "管理当前网络的节点" },
			{ "control", "control.png", this, Boolean.TRUE, "control", "无线控制VD设备节点" },
			{ "params", "params.png", this, Boolean.TRUE, "params", "设置默认检测算法参数" },
			//{ "tracer", "tracer.png", this, Boolean.TRUE, "tracer", null },
			{ null },
			{ "exit", "exit.png", this, Boolean.TRUE, "exit", null }
	};

	private void createToolBar() {
		final String iconDir = "res/icon/";

		toolBar = new JToolBar();
		toolBar.setFloatable(false);
		toolBar.setBorder(new BevelBorder(BevelBorder.RAISED));
		toolBar.setPreferredSize(new Dimension(780,30));

		toolBarButtons = new JButton[toolBarData.length];
		for (int i = 0; i < toolBarData.length; i++) {
			Object[] item = toolBarData[i];

			if (item[0] != null) {
				toolBarButtons[i] = new JButton(I18N.get("toolbar." + item[0]));
				if (item[1] != null) toolBarButtons[i].setIcon(new ImageIcon(iconDir + (String)item[1]));
				toolBarButtons[i].addActionListener((ActionListener)item[2]);
				toolBarButtons[i].setEnabled(((Boolean)item[3]).booleanValue());
				toolBarButtons[i].setActionCommand("toolbar." + (String)item[4]);
				if (item[5] != null) toolBarButtons[i].setToolTipText((String)item[5]);
				//if (item[5] != null) toolBarButtons[i].setToolTipText(I18N.get("toolbar.tooltip." + (String)item[5]));
				toolBar.add(toolBarButtons[i]);
			} else {
				toolBar.addSeparator();
			}
		}

	}

	private final int FORWARD  = 1;
	private final int BACKWARD = 2;

	private void jumpToMatchRow(String patt, int direction) {
		int[] rows = descriptionTable.getSelectedRows();
		int curRow = (rows.length > 0) ? rows[0] : 0;

		if (direction == FORWARD) {
			for (int r = curRow+1; r < descriptionTable.getRowCount(); r++) {
				for (int c = 0; c < descriptionTable.getColumnCount(); c++) {
					String cell = (String) descriptionTable.getValueAt(r, c);
					if (cell.indexOf(patt) != -1) {
						descriptionTable.getSelectionModel().setSelectionInterval(r, r);
						messageTable.getSelectionModel().setSelectionInterval(r, r);
						return;
					}
				}
			}
		}
		else if (direction == BACKWARD) {
			for (int r = curRow-1; r >= 0; r--) {
				for (int c = 0; c < descriptionTable.getColumnCount(); c++) {
					String cell = (String) descriptionTable.getValueAt(r, c);
					if (cell.indexOf(patt) != -1) {
						descriptionTable.getSelectionModel().setSelectionInterval(r, r);
						messageTable.getSelectionModel().setSelectionInterval(r, r);
						return;
					}
				}
			}
		}
	}

	private void clear() {
		dataCount = 0;
		descriptionTableModel.setRowCount(0);
		messageTableModel.setRowCount(0);
		//networkMap.reset();
	}

	private void save() {
		String fname = fnameFmt.format(new Date()) + ".csv";
		debug("SnifferApp: save to " + fname);

		try {
			// write csv header
			File f = new File(fname);
			if (!f.exists()) {
				FileWriter fout = new FileWriter(fname);
				fout.write("lineno,time,rssi,moteid,group,type,packet,description\n");
				fout.close();
			}

			// write csv content
			FileWriter fout = new FileWriter(fname, true);
			for (int i = 0; i < descriptionTable.getRowCount(); i++) {
				String lineno = (String) descriptionTable.getValueAt(i, 0);
				String time = (String) descriptionTable.getValueAt(i, 1);
				String rssi = (String) descriptionTable.getValueAt(i, 2);
				String moteid = (String) descriptionTable.getValueAt(i, 3);
				String group = (String) descriptionTable.getValueAt(i, 4);
				String type = (String) descriptionTable.getValueAt(i, 5);
				String packet = (String) messageTable.getValueAt(i, 1);
				String desc = (String) descriptionTable.getValueAt(i, 6);
				fout.write(lineno+","+time+","+rssi+","+moteid+","+group+","+type+","+packet+","+desc+"\n");
			}
			fout.close();
		} catch(Exception e) {
			e.printStackTrace();
		}

		JOptionPane.showMessageDialog(this, "saved " + descriptionTable.getRowCount()
		+ " records into " + fname);
	}

	private JComboBox moteComboBox;
	private JLabel moteLabel;

	private final SimpleDateFormat fnameFmt = new SimpleDateFormat("yyyyMMdd_HHmmss");
	private final SimpleDateFormat timeFmt = new SimpleDateFormat("MM/dd HH:mm:ss.SSS");
	private final SimpleDateFormat logtimeFmt = new SimpleDateFormat("[yyyy-MM-dd HH:mm:ss,SSS]");
	private boolean raw_rssi = false;

	private Logger log = LoggerFactory.getLogger(SnifferApp.class);

	private int maxRowCount = Config.getInt("table-max-count");
	private int dataCount = 0;

	private boolean bytesEqu(byte[] a, byte[] b) {
		if (a.length != b.length)
			return false;
		for (int i = 0; i < a.length; i++) {
			if (a[i] != b[i])
				return false;
		}
		return true;
	}

	private int getLEUInt16(byte[] packet, int beg) {
		return ((packet[beg+1] & 0xff) << 8) + (packet[beg] & 0xff);
	}

	private int getBEUInt16(byte[] packet, int beg) {
		return ((packet[beg] & 0xff) << 8) + (packet[beg + 1] & 0xff);
	}

	private ParkTestRec toParkTestRec(Packet p) {
		ParkTestRec rec = new ParkTestRec();
		rec.atime = new Date();
		rec.nodeid = p.getField("id").toInt();
		rec.seqno = p.getField("seq").toInt();
		rec.state = p.getField("state").toInt();
		rec.rssi = 0;
		//rec.mag1x = p.getField("x1").toInt();
		//rec.mag1y = p.getField("y1").toInt();
		//rec.mag1z = p.getField("z1").toInt();
		rec.mag2x = p.getField("x2").toInt();
		rec.mag2y = p.getField("y2").toInt();
		rec.mag2z = p.getField("z2").toInt();
		return rec;
	}

	@Override
	public void packetReceived(byte[] pkt) {
		// update count in status bar
		rcvdCount += 1;
		updateStatus();
		//log.info(String.format("rcvd: (%d) %s", pkt.length, strBytes(pkt)));

		try {
			if ("raw".equals(Config.get("serial-proto"))) {
				// parse packet
				byte[] packet = pkt;
				Packet p = Main.parser.parse(packet);

				// write parking packet to data file
				if (p != null && "park test".equals(p.getName())) {
					ParkTestRec object = toParkTestRec(p);
					String text = JSON.toJSONString(object) + "\r\n";
					FileUtils.writeStringToFile(dataFile, text, true);
				}
				if (tabMagnet != null)
					tabMagnet.addPacket(p, new Date());

				// create description from parsed packet
				String[] desc = getDesc(p);
				log.info(String.format("rcvd@ch%d: (%d) %s", activeChan, packet.length, strBytes(packet)));
				log.info(String.format("<%d>:%16s: %s", desc[0], desc[1]));
				addPacket(System.currentTimeMillis(), activeChan, packet, desc, 0);
			}
			else if ("hdlc".equals(Config.get("serial-proto"))) {
				if (pkt[0] == Const.SNIFFER_MSG) { // sniff_msg
					byte[] packet = BytesUtil.getBytes(pkt, 5, pkt.length-6);
					//int moteid = getLEUInt16(packet, 2);
					int rssi = BytesUtil.getInt8(pkt, 2);

					//if (packet.length >= 4) {
					//    Mote m = MoteManager.getInstance().getMote(moteid);
					//    if (m == null) {
					//        m = new Mote();
					//        m.setSaddr(moteid);
					//    }
					//    int ofs = 0;
					//    if (packet.length >= 41 && packet[18] == 0x21) { // hbeat data
					//        if (packet.length == 41) ofs = 21;
					//        if (packet.length == 42) ofs = 22;
					//    } else if (packet.length >= 26 && packet[18] == 0x11) { // mote ident
					//        ofs = 21;
					//    }
					//    if (ofs != 0) {
					//        byte[] meterno = new byte[6];
					//        for (int i = 0; i < 6; i++)
					//            meterno[i] = (byte) (packet[ofs + 5 - i] & 0xff);
					//        Mote mm = MoteManager.getInstance().getMote(meterno);
					//        if (mm != null && mm != m) {
					//            mm.setSaddr(moteid);
					//            MoteManager.getInstance().removeMote(m);
					//        }
					//    }
					//}

					// parse packet
					Packet p = Main.parser.parse(packet);
					Date now = new Date();

					// create description from parsed packet
					String[] desc = getDesc(p);
					int moteid = (p != null ? p.getField("from").toInt() : 0);
					log.info(String.format("rcvd@ch%d: (%d) %s", activeChan, packet.length, strBytes(packet)));
					log.info(String.format("<%d>:%16s: %s", moteid, desc[0], desc[1]));
					addPacket(System.currentTimeMillis(), activeChan, packet, desc, rssi);

					// write parking packet to data file
					if (p != null && "park test".equals(p.getName())) {
						ParkTestRec object = toParkTestRec(p);
						String text = JSON.toJSONString(object) + "\r\n";
						FileUtils.writeStringToFile(dataFile, text, true);
					}
					if (tabMagnet != null)
						tabMagnet.addPacket(p, now);

					if (tabAnalyse != null)
						tabAnalyse.addPacket(p, now);

					if (tabParkEvent != null)
						tabParkEvent.addPacket(p, now);

					if (tracerDlg != null && p != null)
						tracerDlg.packetReceived(p);
				}
				else if (pkt[0] == Const.SNIFFER_DATA) { // sniff_data
					if (pkt[2] == Const.SNIFFER_DATA_VER) {
						firmwareVer = String.format("%d.%d.%d", BytesUtil.getUInt8(pkt, 4), BytesUtil.getUInt8(pkt, 5), BytesUtil.getBEUInt16(pkt, 6));
						firmwareCommit = BytesUtil.getString(pkt, 8, 7);
						firmwareBuild = BytesUtil.getString(pkt, 16, 8);
						log.info(String.format("got version:%s, commit:%s, build:%s", firmwareVer, firmwareCommit, firmwareBuild));
					}
					else if (pkt[2] == Const.SNIFFER_DATA_START) {
						Main.gui.showOutput(String.format("已重启"));
					}
					else if (pkt[2] == Const.SNIFFER_DATA_CHAN) {
						activeChan  = BytesUtil.getUInt8(pkt, 4);
						Main.gui.showOutput(String.format("正在信道%d监听", activeChan));

						channelCombo.removeActionListener(channelListener);
						channelCombo.setSelectedItem(Integer.valueOf(activeChan));
						channelCombo.addActionListener(channelListener);
					}
					else if (pkt[2] == Const.SNIFFER_DATA_PWR) {
						activePower  = BytesUtil.getUInt8(pkt, 4);
						Main.gui.showOutput(String.format("发送功率%ddbm", activePower));

						txpowerCombo.removeActionListener(txpowerListener);
						txpowerCombo.setSelectedItem(Integer.valueOf(activePower));
						txpowerCombo.addActionListener(txpowerListener);
					}
					else {
						log.warn(String.format("unknown sniff data 0x%02X", pkt[2]));
					}
				}
				else if (pkt[0] == Const.SNIFFER_CMD) {
					// nothing need to do
				}
				else if (pkt[0] == Const.SNIFFER_ACK) {
					// nothing need to do
				}
				else {
					log.warn(String.format("unknown message, type 0x%02X", pkt[0]));
				}
			}
		} catch(Exception e) {
			log.warn("", e);
		}
	}

	private void addPacket(long rcvT, int chan, byte[] packet) {
		Packet p = Main.parser.parse(packet);
		if (p == null)
			return;
		addPacket(rcvT, chan, packet, getDesc(p), 0);
	}

	private void addPacket(long rcvT, int chan, byte[] packet, String[] desc, int rssiDbm) {
		try {
			// update group combo
			//int group = Util.getUInt8(packet,6);
			//String groupStr = "0x" + Long.toHexString(group).toUpperCase();
			//updateCombo(groupComboBox, groupStr);
			String groupStr = "";

			if (packet.length < 9) // 802.15.4
				return; // too short packet, may be ACK

			String lineno = String.valueOf(dataCount);
			dataCount += 1;

			// update mote combo
			//int moteid = getLEUInt16(packet, 2);
			int moteid = getBEUInt16(packet, 7); // 802.15.4
			updateCombo(moteComboBox, String.valueOf(moteid));

			// extract packet rssi
			String rssi = "";
			/*if (raw_rssi) {
                    int rssiRaw = Util.getUInt16(packet, packet.length-2);
                    double rssiVal = Util.int2dbm(rssiRaw);
                    rssi = String.valueOf(rssiVal);
                    rssi = rssi.substring(0, rssi.indexOf(".")+2);
                } else {
                    int rssiRaw = Util.getInt16(packet, packet.length-2);
                    rssi = String.valueOf(rssiRaw);
                }*/
			rssi = String.valueOf(rssiDbm);

			// add row for message table
			String str = pktstr(packet);
			//str += " (" + packet.length + ")";
			if (new_at_top) {
				messageTableModel.insertRow(0, new String[]{lineno, String.valueOf(packet.length), str});
			} else {
				messageTableModel.addRow(new String[]{lineno, String.valueOf(packet.length), str});
				int msgN = messageTable.getModel().getRowCount();
				Rectangle cellLocation1 = messageTable.getCellRect(msgN-1, 0, false);
				messageTableScrollPane.getVerticalScrollBar().setValue(cellLocation1.y);
			}
			if (messageTableModel.getRowCount() >= maxRowCount + (maxRowCount >> 2)) {
				messageTableModel.setRowCount(maxRowCount);
			}

			// update mote info
			String time = timeFmt.format(rcvT);
			Mote m = motemgr.getMote(moteid);
			if (m == null) {
				try {
					Packet p = Main.parser.parse(packet);
					if (p != null) {
						if ("netcmd req".equals(p.getName())
								|| "reboot".equals(p.getName()))
						{
							byte[] mac = p.getField("mac").value;
							m = motemgr.getMoteByMAC(mac);
							if (m == null) {
								byte[] dev = new byte[6];
								System.arraycopy(mac, 2, dev, 0, 6);
								m = new Mote();
								m.setMac(mac);
								m.setDev(dev);
								m.setSaddr(moteid);
								motemgr.addMote(m);
								motemgr.save();
							}
							if (moteid != m.getSaddr()) {
								m.setSaddr(moteid);
								motemgr.save();
							}
						}
						else if ("connect".equals(p.getName()))
						{
							byte[] dev = p.getField("dev").value;
							m = motemgr.getMoteByDev(dev);
							if (m == null) {
								byte[] mac = new byte[8];
								System.arraycopy(dev, 0, mac, 2, 6);
								m = new Mote();
								m.setMac(mac);
								m.setDev(dev);
								m.setSaddr(moteid);
								motemgr.addMote(m);
								motemgr.save();
							}
							if (moteid != m.getSaddr()) {
								m.setSaddr(moteid);
								motemgr.save();
							}
						}
					}
				} catch(Exception e) {
					log.warn("fail manage mote: " + e.getMessage());
				}
			}

			// add row for description table
			String[] row = new String[] {
					lineno, time, rssi, String.valueOf(chan), groupStr,
					((m != null) ? m.getName() : "N/A"), String.format("<%d>", moteid),
					desc[0], desc[1]
			};
			if (new_at_top) {
				descriptionTableModel.insertRow(0, row);
			} else {
				descriptionTableModel.addRow(row);
				int descN = descriptionTable.getModel().getRowCount();
				Rectangle cellLocation = descriptionTable.getCellRect(descN-1, 0, false);
				descriptionTableScrollPane.getVerticalScrollBar().setValue(cellLocation.y);
			}
			if (descriptionTableModel.getRowCount() > maxRowCount + (maxRowCount >> 2)) {
				descriptionTableModel.setRowCount(maxRowCount);
			}

		} catch (Exception e) {
			e.printStackTrace();
			log.warn("fail to add packet", e);
		}
	}

	private void updateCombo(JComboBox combo, String in) {
		if (combo == null)
			return;
		for (int i = 0; i < combo.getItemCount(); i++) {
			String item = (String) combo.getItemAt(i);
			if (in.equals(item))
				return;
		}
		combo.addItem(in);
	}

	private String getStr(byte[] packet, int beg, int len) {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < len; i++) {
			if (packet[beg + i] == 0)
				break;
			sb.append((char)packet[beg+i]);
		}
		return sb.toString();
	}

	private String getVerNo(byte[] packet, int ofs) {
		int major = (packet[ofs + 1] & 0xff);
		int minor = (packet[ofs + 2] & 0xff);
		int rev = (packet[ofs + 3] & 0xff);
		return String.format("v%d.%d.%d", major, minor, rev);
	}

	private String textBytes(byte[] packet, int ofs, int len) {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < len; i++) {
			if ("req ".equals(sb.toString()) || "rep ".equals(sb.toString()))
				sb.append((packet[ofs + i] & 0xff));
			else
				sb.append((char)packet[ofs + i]);
		}
		return sb.toString();
	}

	private String[] getDesc(Packet p) {
		String[] texts = new String[2];
		StringBuilder sb = new StringBuilder();

		try {
			if (p != null) {
				texts[0] = p.getName();
				texts[1] = p.toDescription(", ");
			} else {
				texts[0] = "N/A";
				texts[1] = "";
			}

			// add mote name to description
			if (Config.getBool("desc-with-name")) {
				Pattern patt = Pattern.compile("([a-zA-Z]+)=([a-zA-Z0-9 ]+)");
				Matcher m = null;
				for (String str : texts[1].split(", ")) {
					if (sb.length() > 0)
						sb.append(", ");
					m = patt.matcher(str);
					if (m.find()) {
						String name = m.group(1);
						if (name.equals("to") || name.equals("from")
								|| name.equals("erecver") || name.equals("esender")
								|| name.equals("recver") || name.equals("sender")) {
							int moteid = Integer.parseInt(m.group(2));
							Mote mote = MoteManager.getInstance().getMote(moteid);
							if (mote == null) {
								byte[] meter = new byte[]{0x02,0x01,0x30,(byte) 0x80,0x00,0x00};
								meter[4] = (byte) ((((moteid / 1000) % 10) << 4) + ((moteid / 100) % 10));
								meter[5] = (byte) ((((moteid / 10) % 10) << 4) + ((moteid) % 10));
								mote = MoteManager.getInstance().getMoteByDev(meter);
								if (mote != null)
									mote.setSaddr(moteid);
							}
							if (mote != null)
								sb.append(name).append("=")
								.append(moteid).append("{").append(mote.getName()).append("}");
							else
								sb.append(str);
						} else {
							sb.append(str);
						}
					} else {
						sb.append(str);
					}
					texts[1] = sb.toString();
				}
			}
		} catch (Exception e) {
			log.warn("invalid packet " + strBytes(p.getBytes()), e);
		}

		return texts;
	}

	private String strResult(int res) {
		final String[] strs = new String[] {
				"SUCCESS",
				"FAIL"
		};
		if (res >= 0 && res < strs.length)
			return strs[res];
		return String.format("0x%02x", res);
	}

	private String strHBeatType(int type) {
		String[] NAMEs = new String[]{"", "hbeat test", "hbeat ack"};
		return NAMEs[type];
	}

	private String strCtpOpt(int opt) {
		StringBuilder sb = new StringBuilder();
		sb.append(String.format("0x%02x/", opt));
		if ((opt & 0x80) > 0) sb.append("PULL");
		if ((opt & 0x40) > 0) sb.append(" ECN");
		return sb.toString();
	}
	private String strDebugLevel(int level) {
		final String levels[] = { "info", "verbose", "warn", "error", "fatal" };
		return (level < levels.length ? levels[level] : "N/A");
	}

	private String strFilename(byte[] packet, int ofs, int len) {
		StringBuilder sb = new StringBuilder();
		int i = ofs;
		do { sb.append((char)packet[i++]); }
		while (packet[i] != 0 && i < ofs+len);
		return sb.toString();
	}

	public void showOutput(String msg) {
		final SimpleDateFormat msecFmt = new SimpleDateFormat("HH:mm:ss.SSS");
		//outputArea.append(msecFmt.format(System.currentTimeMillis()));
		//outputArea.append("  ");
		//outputArea.append(msg);
		//outputArea.append("\r\n");
		Object[] rowData = new Object[2];
		int i = 0;
		rowData[i++] = msecFmt.format(System.currentTimeMillis());
		rowData[i++] = msg;
		outputTableM.insertRow(0, rowData);
	}

	public void showStatusMsg(String msg) {
		hintLabel.setText(msg);
		statusT.restart();
	}

	public void showStatusMsg(String msg, boolean autohide) {
		hintLabel.setText(msg);
		if (autohide)
			statusT.restart();
	}

	public static String pktstr(byte[] packet) {
		String str = "";
		for (int i = 0; i < packet.length; i++) {
			String num = Long.toHexString(packet[i] & 0xff).toUpperCase();
			str += (num.length() < 2 ? "0" : "") + num + " ";
		}
		return str;
	}

	private String strBytes(byte[] packet) {
		return strBytes(packet, 0, packet.length);
	}

	private String strBytes(byte[] packet, int ofs, int len) {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < len; i++)
			sb.append(String.format("%02X", packet[ofs+i])).append(" ");
		return sb.substring(0, sb.length()-1);
	}

	private void debug(String s) {
		if (DEBUG)
			System.out.println(s);
	}

	private JLabel getChanLabel() {
		if(chanLabel == null) {
			chanLabel = new JLabel();
			chanLabel.setText(I18N.get("combo.Channel"));
		}
		return chanLabel;
	}

	private JComboBox getChanCombo() {
		if (channelCombo == null) {
			channelCombo = new JComboBox();
			channelCombo.setPreferredSize(new Dimension(45,22));
			int min = Config.getInt("channel-min");
			int max = Config.getInt("channel-max");
			for (int i = min; i <= max; i++)
				channelCombo.addItem(Integer.valueOf(i));
			channelCombo.setSelectedItem(Integer.valueOf(Config.getInt("radio-channel")));

			channelListener = new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					activeChan = (Integer) channelCombo.getSelectedItem();
					showOutput(String.format("切换到信道%d", activeChan));
					byte[] arg = new byte[]{ (byte)activeChan };
					Main.nwk.sendCommand(Const.SNIFFER_CMD_SETCHAN, arg, true);
				}
			};
			channelCombo.addActionListener(channelListener);
		}
		return channelCombo;
	}

	private JLabel getTXPowerLabel() {
		if(txpowerLabel == null) {
			txpowerLabel = new JLabel();
			txpowerLabel.setText(I18N.get("combo.TXPower"));
		}
		return txpowerLabel;
	}

	private JComboBox getTXPowerCombo() {
		if (txpowerCombo == null) {
			txpowerCombo = new JComboBox();
			txpowerCombo.setPreferredSize(new Dimension(45,22));
			int min = Config.getInt("txpower-min");
			int max = Config.getInt("txpower-max");
			for (int i = min; i <= max; i++)
				txpowerCombo.addItem(Integer.valueOf(i));
			txpowerCombo.setSelectedItem(Integer.valueOf(Config.getInt("radio-txpower")));

			txpowerListener = new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					activePower = (Integer) txpowerCombo.getSelectedItem();
					showOutput(String.format("切换到发送功率%d", activePower));
					byte[] arg = new byte[]{ (byte)activePower };
					Main.nwk.sendCommand(Const.SNIFFER_CMD_SETPWR, arg, true);
				}
			};
			txpowerCombo.addActionListener(txpowerListener);
		}
		return txpowerCombo;
	}

	private JLabel getGroupLabel() {
		if(groupLabel == null) {
			groupLabel = new JLabel();
			groupLabel.setText("PAN");
		}
		return groupLabel;
	}

	private JComboBox getGroupCombo() {
		if(groupComboBox == null) {
			ComboBoxModel jComboBox1Model =
					new DefaultComboBoxModel(
							new String[] { "    " });
			groupComboBox = new JComboBox();
			groupComboBox.setModel(jComboBox1Model);
			groupComboBox.setPreferredSize(new Dimension(50,22));

			groupComboBox.addItemListener(new ItemListener() {
				@Override
				public void itemStateChanged(ItemEvent e) {
					String group = ((String) ((JComboBox)e.getSource()).getSelectedItem()).trim();
					updateFilter(group);
				}
			});
		}
		return groupComboBox;
	}

	private JLabel getMoteLabel() {
		if(moteLabel == null) {
			moteLabel = new JLabel();
			moteLabel.setText(I18N.get("textf.Mote"));
		}
		return moteLabel;
	}

	private JTextField getMoteFilter() {
		if (moteFilterInput == null) {
			moteFilterInput = new JTextField();
			moteFilterInput.setColumns(8);
			moteFilterInput.getDocument().addDocumentListener(new DocumentListener() {
				@Override
				public void removeUpdate(DocumentEvent e) {
					String moteid = moteFilterInput.getText().trim();
					String filter = (moteid.matches("^[0-9]+$")) ? "<"+moteid+">" : "";
					descriptionRowSorter.setRowFilter(RowFilter.regexFilter(filter));
					filterTextField.setEnabled(moteid.length() == 0);
				}

				@Override
				public void insertUpdate(DocumentEvent e) {
					String moteid = moteFilterInput.getText().trim();
					String filter = (moteid.matches("^[0-9]+$")) ? "<"+moteid+">" : "";
					descriptionRowSorter.setRowFilter(RowFilter.regexFilter(filter));
					filterTextField.setEnabled(moteid.length() == 0);
				}

				@Override
				public void changedUpdate(DocumentEvent e) {
				}
			});
		}
		return moteFilterInput;
	}

	private JComboBox getMoteCombo() {
		if(moteComboBox == null) {
			ComboBoxModel jComboBox1Model =
					new DefaultComboBoxModel(
							new String[] { "    " });
			moteComboBox = new JComboBox();
			moteComboBox.setModel(jComboBox1Model);

			moteComboBox.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					JComboBox combo = (JComboBox)e.getSource();
					String moteid = ((String) combo.getSelectedItem()).trim();
					String filter = (moteid.matches("^[0-9]+$")) ? "<"+moteid+">" : "";
					descriptionRowSorter.setRowFilter(RowFilter.regexFilter(filter));
					filterTextField.setEnabled(combo.getSelectedIndex() == 0);
				}
			});
		}
		return moteComboBox;
	}

	private void updateFilter(String group) {
		descriptionFilter = group;
		try {
			descriptionRowSorter.setRowFilter(RowFilter.regexFilter(descriptionFilter));
			debug("set filter to <" + descriptionFilter + ">");
		} catch(PatternSyntaxException e) {
			log.warn("fail set filter: " + group, e);
		}
	}

	private JLabel getSepLabel(int width) {
		JLabel sepLabel2 = new JLabel();
		sepLabel2.setText("");
		sepLabel2.setPreferredSize(new Dimension(width,16));

		return sepLabel2;
	}

	private JLabel getJLabel2() {
		if(sepLabel2 == null) {
			sepLabel2 = new JLabel();
			sepLabel2.setText("");
			sepLabel2.setPreferredSize(new Dimension(40,16));
		}
		return sepLabel2;
	}

	private JLabel getJLabel3() {
		if(filterLabel == null) {
			filterLabel = new JLabel();
			filterLabel.setText(I18N.get("textf.Filter"));
		}
		return filterLabel;
	}

	private JTextField getTableFilter() {
		if(filterTextField == null) {
			filterTextField = new JTextField();
			filterTextField.setColumns(15);
			filterTextField.setText("");

			filterTextField.getDocument().addDocumentListener(new DocumentListener() {
				public void changedUpdate(DocumentEvent e) {
				}
				public void insertUpdate(DocumentEvent e) {
					updateFilter(filterTextField.getText().trim());
				}
				public void removeUpdate(DocumentEvent e) {
					updateFilter(filterTextField.getText().trim());
				}
			});
		}
		return filterTextField;
	}

	private JButton getClearBtn() {
		if(clearBtn == null) {
			clearBtn = new JButton("清除");
			clearBtn.addActionListener(new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					clear();
				}
			});
		}
		return clearBtn;
	}

	private String descriptionFilter = "";
	private MoteMgrDialog managerDlg;
	private MoteCtrlDialog ctrlDlg;
	private TracerDialog tracerDlg;
	private String replay_file;

	private JLabel getJLabel4() {
		if(sepLabel4 == null) {
			sepLabel4 = new JLabel();
			sepLabel4.setText("");
			sepLabel4.setPreferredSize(new Dimension(40,16));
		}
		return sepLabel4;
	}

	public void fireToolBarCmd(String action) {
		if (action.equals("toolbar.start")) {
			toolBarButtons[0].setEnabled(false);
			toolBarButtons[1].setEnabled(true);
			//            nwk.start();
		}
		else if (action.equals("toolbar.stop")) {
			toolBarButtons[0].setEnabled(true);
			toolBarButtons[1].setEnabled(false);
			//            nwk.stop();
		}
		else if (action.equals("toolbar.setting")) {
			CommDialog diag = new CommDialog(this);
			diag.setLocationRelativeTo(this);
			diag.setVisible(true);
		}
		else if (action.equals("toolbar.edit")) {
			editMsgsXml();
		}
		else if (action.equals("toolbar.reload")) {
			Main.parser.reload();
			showStatusMsg(I18N.get("msg.reloadok"));
		}
		else if (action.equals("toolbar.clear")) {
			clear();
			tabAnalyse.clear();
			tabParkEvent.clear();
		}
		else if (action.equals("toolbar.replay")) {
			replay_file = null;
			File file1 = new File(Config.get("replay-dir"));
			JFileChooser chooser = new JFileChooser(file1);
			//FileFilter filter1 = new ExtensionFileFilter("Image file (*.jpg,*.png,*.jpeg)", new String[] { "JPG", "JPEG", "PNG" });
			//chooser.setFileFilter(filter1);
			int returnVal = chooser.showOpenDialog(null);
			if (returnVal == JFileChooser.APPROVE_OPTION) {
				replay_file = chooser.getSelectedFile().getPath();
				Config.set("replay-dir", new File(replay_file).getParent(), true);

				Runnable r = new Runnable() {
					@Override
					public void run() {
						clear();
						try {
							InputStream in = new FileInputStream(new File(replay_file));
							InputStreamReader reader = new InputStreamReader(in, "UTF-8");
							BufferedReader br = new BufferedReader(reader);
							String line = null;
							Pattern p = Pattern.compile("^([0-9-]+ [0-9:,]+) .*rcvd@ch([0-9]+): \\(([0-9]+)\\) ([0-9A-F ]+)$");
							Matcher m = null;
							int count = 0;
							SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss,SSS");

							while ((line = br.readLine()) != null) {
								m = p.matcher(line);
								if (m.find()) {
									try {
										Date t = sdf.parse(m.group(1));
										int chan = Integer.parseInt(m.group(2));
										String spkt = m.group(4);
										String subs[] = spkt.split(" ");
										byte[] pkt = new byte[subs.length];
										for (int i = 0; i < subs.length; i++)
											pkt[i] = (byte)Integer.parseInt(subs[i], 16);

										Packet ppkt = Main.parser.parse(pkt);
										addPacket(t.getTime(), chan, pkt);
										if (tabParkEvent != null)
											tabParkEvent.addPacket(ppkt, t);
										if (tabAnalyse != null)
											tabAnalyse.addPacket(ppkt, t);
										if (tabMagnet != null)
											tabMagnet.addPacket(ppkt, t);
										++count;
									} catch(Exception e) {
										log.warn("fail add: " + line);
									}
								}
							}
							log.debug(String.format("loaded %d packets", count));

							br.close();
							reader.close();
							in.close();
						} catch (Exception e) {
							log.warn("fail replay " + replay_file);
						}
					}
				};
				new Thread(r).start();
			}
		}
		else if (action.equals("toolbar.manager")) {
			if (managerDlg == null)
				managerDlg = new MoteMgrDialog(SnifferApp.this);
			managerDlg.load();
			managerDlg.setLocationRelativeTo(this);
			managerDlg.setVisible(true);
		}
		else if (action.equals("toolbar.control")) {
			ctrlDlg = null;
			ctrlDlg = new MoteCtrlDialog(SnifferApp.this);
			Main.comm.addPacketListener(ctrlDlg);
			MoteManager.getInstance().addView(ctrlDlg);

			Point p = SnifferApp.this.getLocation();
			p.x += Config.getInt("dialog-offset-x");
			p.y += Config.getInt("dialog-offset-y");
			ctrlDlg.setLocation(p.x, p.y);
			ctrlDlg.setVisible(true);
		}
		else if (action.equals("toolbar.params")) {
			showAlgoDialog();
		}
		else if (action.equals("toolbar.tracer")) {
			tracerDlg = new TracerDialog(SnifferApp.this);
			tracerDlg.setLocationRelativeTo(this);
			tracerDlg.setVisible(true);
		}
		else if (action.equals("toolbar.exit")) {
			exit();
		}
		else if (action.equals("sendPacket")) {
			String[] spkt = packetEdit.getText().split(" ");
			byte[] pkt = new byte[spkt.length];
			for (int i = 0; i < pkt.length; i++)
				pkt[i] = (byte) Integer.parseInt(spkt[i], 16);

			showOutput(String.format("发送数据%d字节", pkt.length));
			Main.nwk.sendCommand(Const.SNIFFER_CMD_SENDPKT, pkt, true);
		}
		else {
			log.warn("unknown action " + action);
		}
	}

	private void editMsgsXml() {
		try {
			String cmdStr = "cmd /c notepad.exe packet.xml" ;
			Runtime.getRuntime().exec(cmdStr);
		} catch(Exception e) {
			String message = "无法用\"记事本\"打开消息定义文件";
			JOptionPane.showMessageDialog(this, message);
		}
	}

	private void getFirmwareVer() {
		firmwareCommit = null;
		firmwareBuild = null;

		Main.nwk.getFirmwareVer();

		long deadline = System.currentTimeMillis() + 1000;
		while (firmwareCommit == null && System.currentTimeMillis() < deadline) {
			try { Thread.sleep(50); } catch(InterruptedException e) {}
		}
	}

	private void showAlgoDialog() {
		AlgoDialog dlg = new AlgoDialog(SnifferApp.this);
		dlg.setLocationRelativeTo(SnifferApp.this);
		dlg.setVisible(true);
	}

	@Override
	public void actionPerformed(ActionEvent arg0) {
		String action = arg0.getActionCommand();
		fireToolBarCmd(action);
	}

}
