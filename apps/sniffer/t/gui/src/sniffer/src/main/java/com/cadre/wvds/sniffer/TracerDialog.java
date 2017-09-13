package com.cadre.wvds.sniffer;

import javax.swing.JDialog;
import javax.swing.JPanel;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Rectangle;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import javax.swing.JFrame;
import javax.swing.JSplitPane;
import javax.swing.JScrollPane;
import javax.swing.BoxLayout;
import javax.swing.JTable;
import javax.swing.JButton;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.DefaultTableModel;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.LineIterator;

import net.skaldchen.commons.packet.Packet;
import net.skaldchen.commons.packet.PacketDef;
import net.skaldchen.commons.utils.JTableUtil;

import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;

public class TracerDialog extends JDialog implements MVCView {

    private final int W = 640;
    private final int H = 540;

    private JTable filelistTable;
    private DefaultTableModel filelistTableM;
    private JTable packetTable;
    private DefaultTableModel packetTableM;
    private ArrayList<Packet> packetTableData;
    private JTable traceTable;
    private DefaultTableModel traceTableM;
    private JTable fileTable;
    private DefaultTableModel fileTableM;
    private JScrollPane scrollPane_3;

    private int current_file = -1;

    public TracerDialog(JFrame parent) {
        super(parent, true);

        this.setTitle("Tracer");
        this.setSize(W, H);
        this.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);

        JPanel panel = new JPanel();
        getContentPane().add(panel, BorderLayout.NORTH);

        JButton btnNewButton = new JButton("Close");
        //panel.add(btnNewButton);

        JSplitPane splitPane = new JSplitPane();
        splitPane.setResizeWeight(0.4);
        splitPane.setOrientation(JSplitPane.VERTICAL_SPLIT);
        getContentPane().add(splitPane, BorderLayout.CENTER);

        JPanel panel_1 = new JPanel();
        splitPane.setLeftComponent(panel_1);
        panel_1.setLayout(new BoxLayout(panel_1, BoxLayout.Y_AXIS));
        panel_1.setPreferredSize(new Dimension(640, 200));

        JPanel panel_2 = new JPanel();
        panel_1.add(panel_2);
        panel_2.setLayout(new BorderLayout());

        JScrollPane scrollPane = new JScrollPane();
        panel_2.add(scrollPane, BorderLayout.CENTER);

        filelistTable = new JTable();
        filelistTableM = new DefaultTableModel(
                new Object[][] {
                },
                new String[] {
                    "fid", "path"
                }
                ) {
            Class[] columnTypes = new Class[] {
                Integer.class, String.class
            };
            public Class getColumnClass(int columnIndex) {
                return columnTypes[columnIndex];
            }
        };
        filelistTable.setModel(filelistTableM);
        scrollPane.setViewportView(filelistTable);
        int[] filelistTableWidths = new int[]{ 30, W-120 };
        JTableUtil.fitTableColumns(filelistTable, filelistTableWidths);

        List<TraceFile> files = Main.tracemgr.getFiles();
        for (TraceFile f : files) {
            Object[] row = new Object[2];
            row[0] = Integer.valueOf(f.getFid());
            row[1] = f.getPath();
            filelistTableM.addRow(row);
        }

        JPanel panel_3 = new JPanel();
        panel_3.setLayout(new BoxLayout(panel_3, BoxLayout.Y_AXIS));
        JButton addBtn = new JButton("Add");
        addBtn.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                TracerFileDialog dlg = new TracerFileDialog(TracerDialog.this);
                dlg.setLocationRelativeTo(TracerDialog.this);
                dlg.setVisible(true);
            }
        });
        panel_3.add(addBtn);
        addBtn.setPreferredSize(new Dimension(60, 20));
        JButton editBtn = new JButton("Edit");
        editBtn.setEnabled(false);
        editBtn.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
            }
        });
        panel_3.add(editBtn);
        editBtn.setPreferredSize(new Dimension(60, 20));
        JButton delBtn = new JButton("Del");
        delBtn.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                int[] rows = filelistTable.getSelectedRows();
                for (int i = 0; i < rows.length; i++) {
                    String file = (String) filelistTable.getValueAt(rows[i], 1);
                    Main.tracemgr.removeFile(file);
                    Main.tracemgr.save();
                    filelistTableM.removeRow(rows[i]);
                }
            }
        });
        panel_3.add(delBtn);
        delBtn.setPreferredSize(new Dimension(60, 20));
        panel_2.add(panel_3, BorderLayout.EAST);

        JScrollPane scrollPane_1 = new JScrollPane();
        panel_1.add(scrollPane_1);

        packetTable = new JTable();
        packetTableM = new DefaultTableModel(new Object[][] {}, new String[] {
            "time", "description", "packet" }) {
            Class[] columnTypes = new Class[] { Object.class, String.class,
                String.class };

            public Class getColumnClass(int columnIndex) {
                return columnTypes[columnIndex];
            }
        };
        packetTableData = new ArrayList<Packet>();
        packetTable.setModel(packetTableM);
        scrollPane_1.setViewportView(packetTable);
        int[] packetTableWidths = new int[]{ 80, W/2, W/2-90};
        JTableUtil.fitTableColumns(packetTable, packetTableWidths);

        packetTable.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        packetTable.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
            @Override
            public void valueChanged(ListSelectionEvent e) {
                int[] rows = packetTable.getSelectedRows();
                if (rows.length < 1) return;
                Packet pkt = packetTableData.get(rows[0]);
                //System.out.println("click " + pkt.toDescription(","));

                PacketDef def = Main.parser.getPacketDef("trace");
                int ofs = def.getField("entries").offset;
                byte[] data = pkt.getBytes();
                traceTableM.setRowCount(0);
                for (int i = 0; i < Const.TRACER_NUM_PER_MSG; i++) {
                    int fid = data[ofs + 4 * i + 1];
                    int lineno = (data[ofs + 4 * i + 2] & 0xff) + ((data[ofs + 4 * i + 3] & 0xff) << 8);
                    String[] row = new String[2];
                    row[0] = String.valueOf(fid);
                    row[1] = String.valueOf(lineno);
                    traceTableM.addRow(row);
                }
            }
        });

        JSplitPane splitPane_1 = new JSplitPane();
        splitPane_1.setResizeWeight(0.15);
        splitPane.setRightComponent(splitPane_1);

        JScrollPane scrollPane_2 = new JScrollPane();
        splitPane_1.setLeftComponent(scrollPane_2);

        traceTable = new JTable();
        traceTableM = new DefaultTableModel(
                new Object[][] {
                },
                new String[] {
                    "fid", "lineno"
                }
                ) {
            Class[] columnTypes = new Class[] {
                Integer.class, Integer.class
            };
            public Class getColumnClass(int columnIndex) {
                return columnTypes[columnIndex];
            }
        };
        traceTable.setModel(traceTableM);
        scrollPane_2.setViewportView(traceTable);

        traceTable.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        traceTable.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
            @Override
            public void valueChanged(ListSelectionEvent e) {
                int[] rows = traceTable.getSelectedRows();
                if (rows.length < 1) return;
                int fid = Integer.parseInt((String) traceTable.getValueAt(rows[0], 0));
                int lineno = Integer.parseInt((String)traceTable.getValueAt(rows[0], 1));
                //System.out.println("click file " + fid + ", line " + lineno);

                if (fid != current_file) {
                    String fname = null;

                    for (int i = 0; i < filelistTable.getRowCount(); i++) {
                        int f_id = ((Integer) filelistTable.getValueAt(i, 0)).intValue();
                        if (f_id == fid) {
                            fname = (String) filelistTable.getValueAt(i, 1);
                            break;
                        }
                    }

                    //System.out.println("load file " + fname);
                    File f = new File(fname);
                    if (!f.exists()) {
                        System.err.println("file not exist: " + fname);
                        return;
                    }
                    try {
                        LineIterator it = FileUtils.lineIterator(f);
                        String[] row = new String[2];
                        int line = 1;
                        fileTableM.setRowCount(0);
                        while (it.hasNext()) {
                            row[0] = String.valueOf(line++);
                            row[1] = it.next();
                            fileTableM.addRow(row);
                        }
                        current_file = fid;
                    } catch(Exception e1) {
                        e1.printStackTrace();
                    }
                }

                int r = lineno - 1;
                fileTable.getSelectionModel().setSelectionInterval(r, r);
                Rectangle cellLocation = fileTable.getCellRect(r - 5, 0, false);
                scrollPane_3.getVerticalScrollBar().setValue(cellLocation.y);
            }
        });

        scrollPane_3 = new JScrollPane();
        splitPane_1.setRightComponent(scrollPane_3);

        fileTable = new JTable();
        fileTableM = new DefaultTableModel(
                new Object[][] {
                },
                new String[] {
                    "lineno", "text"
                }
                ) {
            Class[] columnTypes = new Class[] {
                Integer.class, String.class
            };
            public Class getColumnClass(int columnIndex) {
                return columnTypes[columnIndex];
            }
        };
        fileTable.setModel(fileTableM);
        scrollPane_3.setViewportView(fileTable);
        int[] fileTableWidths = new int[]{ 50, W-160 };
        JTableUtil.fitTableColumns(fileTable, fileTableWidths);

        Main.tracemgr.addView(this);
    }

    public void packetReceived(Packet pkt) {
        if (pkt == null || !"trace".equals(pkt.getName())) {
            System.err.println("not trace packet");
            return;
        }

        String[] row = new String[3];
        row[0] = msecFmt.format(new Date());
        row[1] = pkt.toDescription(", ");
        row[2] = Packet.strBytes(pkt.getBytes());
        packetTableM.addRow(row);
        packetTableData.add(pkt);
    }

    private final SimpleDateFormat msecFmt = new SimpleDateFormat("HH:mm:ss,SSS");

    @Override
    public void dataAdded(Object o) {
        if (o instanceof TraceFile) {
            TraceFile tf = (TraceFile)o;
            Object[] row = new Object[2];
            row[0] = Integer.valueOf(tf.getFid());
            row[1] = tf.getPath();
            filelistTableM.addRow(row);
        }
    }

    @Override
    public void dataRemoved(Object o) {
        if (o instanceof TraceFile) {
            // nothing to do
        }
    }

    @Override
    public void dataChanged(Object o) {

    }

}
