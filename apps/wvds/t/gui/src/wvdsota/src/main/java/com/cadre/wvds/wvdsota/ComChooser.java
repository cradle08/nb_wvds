package com.cadre.wvds.wvdsota;

import gnu.io.CommPortIdentifier;
import net.skaldchen.commons.utils.ComUtil;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Enumeration;

import javax.swing.BorderFactory;
import javax.swing.ComboBoxModel;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.border.BevelBorder;

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
@SuppressWarnings("serial")
public class ComChooser extends javax.swing.JDialog implements ActionListener {

    private final int WIDTH  = 280;
    private final int HEIGHT = 210;

    private JPanel jPanel1;
    private JButton jButton1;
    private JLabel jLabel3;
    private JComboBox<String> jComboBox2;
    private JLabel jLabel2;
    private JComboBox<String> jComboBox1;
    private JLabel jLabel1;
    private JButton jButton2;
    private JPanel jPanel2;

    public ComChooser() {
        initGUI();
        load();
    }

    private void initGUI() {
        try {
            {
                jPanel2 = new JPanel();
                getContentPane().add(jPanel2, BorderLayout.SOUTH);
                {
                    jButton1 = new JButton();
                    jPanel2.add(jButton1);
                    jButton1.setText("确定");
                    jButton1.setPreferredSize(new Dimension(60,22));
                    jButton1.setActionCommand("ok");
                    jButton1.addActionListener(this);
                }
                {
                    jButton2 = new JButton();
                    jPanel2.add(jButton2);
                    jButton2.setText("退出");
                    jButton2.setPreferredSize(new Dimension(60,22));
                    jButton2.setActionCommand("quit");
                    jButton2.addActionListener(this);
                }
            }
            {
                jPanel1 = new JPanel();
                getContentPane().add(jPanel1, BorderLayout.CENTER);
                jPanel1.setBorder(BorderFactory.createBevelBorder(BevelBorder.RAISED));
                jPanel1.setLayout(null);
                {
                    jLabel1 = new JLabel();
                    jPanel1.add(jLabel1);
                    jLabel1.setText("串口号:");
                    jLabel1.setBounds(25, 54, 70, 15);
                    jLabel1.setHorizontalAlignment(SwingConstants.TRAILING);
                }
                {
                    ComboBoxModel<String> jComboBox1Model =
                        new DefaultComboBoxModel<String>(
                                new String[] { "Item One", "Item Two" });
                    jComboBox1 = new JComboBox<String>();
                    jPanel1.add(jComboBox1);
                    jComboBox1.setModel(jComboBox1Model);
                    jComboBox1.setBounds(100, 51, 90, 22);
                }
                {
                    jLabel2 = new JLabel();
                    jPanel1.add(jLabel2);
                    jLabel2.setText("波特率:");
                    jLabel2.setBounds(25, 89, 70, 15);
                    jLabel2.setHorizontalAlignment(SwingConstants.TRAILING);
                }
                {
                    String[] BAUDs = { "115200", "57600", "38400", "19200", "9600" };
                    ComboBoxModel<String> jComboBox2Model =
                        new DefaultComboBoxModel<String>(BAUDs);
                    jComboBox2 = new JComboBox<String>();
                    jPanel1.add(jComboBox2);
                    jComboBox2.setModel(jComboBox2Model);
                    jComboBox2.setBounds(100, 86, 90, 22);
                }
                {
                    jLabel3 = new JLabel();
                    jPanel1.add(jLabel3);
                    jLabel3.setText("请选择设备连接的正确串口:");
                    jLabel3.setBounds(14, 22, 204, 15);
                }
            }

            setTitle("串口选择");
            setSize(WIDTH, HEIGHT);
            setAlwaysOnTop(true);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void load() {
        Enumeration<?> e = CommPortIdentifier.getPortIdentifiers();
        jComboBox1.removeAllItems();
        while (e.hasMoreElements()) {
            CommPortIdentifier com = (CommPortIdentifier) e.nextElement();
            if (com.getPortType() == CommPortIdentifier.PORT_SERIAL)
                jComboBox1.addItem(com.getName());
        }
    }

    public void selectCOM(String com) {
    	jComboBox1.setSelectedItem(com);
    }

    public void selectBaud(String baud) {
    	jComboBox2.setSelectedItem(baud);
    }

    @Override
    public void actionPerformed(ActionEvent e) {
        String action = e.getActionCommand();
        if (action.equals("ok")) {
			String com = (String) jComboBox1.getSelectedItem();
			if (ComUtil.avail(com)) {
				Main.com_name = (String) jComboBox1.getSelectedItem();
				Main.com_baud = Integer.parseInt((String) jComboBox2.getSelectedItem());
				dispose();
			}
			else {
				String message = "串口" + com + "已被占用，可能原因及措施：\n"
						+ "1) 其他程序已使用该串口，请关闭该程序；\n"
						+ "2) 选择了错误的串口，请选择正确串口。";
				JOptionPane.showMessageDialog(this, message);
			}
        }
        else if (action.equals("quit")) {
            dispose();
            System.exit(0);
        }
    }

}
