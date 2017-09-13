package com.cadre.wvds.sniffer;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Enumeration;

import gnu.io.CommPortIdentifier;

import javax.swing.BorderFactory;
import javax.swing.ButtonGroup;
import javax.swing.ComboBoxModel;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JTextField;
import javax.swing.border.BevelBorder;

import net.skaldchen.commons.utils.Config;

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
public class CommDialog extends JDialog implements ActionListener {
    private JPanel jPanel1;
    private JRadioButton localRadioButton;
    private JComboBox comComboBox;
    private JLabel jLabel1;
    private JButton cancelButton;
    private JButton okButton;
    private JTextField portTextField;
    private JTextField serverTextField;
    private JComboBox baudComboBox;
    private JRadioButton remoteRadioButton;
    private JPanel jPanel2;

    private final int MODE_LOCAL  = 0;
    private final int MODE_REMOTE = 1;

    private final String[] BAUDS = {"115200", "57600", "38400", "19200", "9600"};

    public CommDialog(JFrame owner) {
        super(owner, true);
        initGUI();
        loadCOM();
        setMode();
    }

    private void initGUI() {
        try {
            {
                this.setSize(320, 240);
                this.setTitle("Comm setting");
                {
                    jPanel1 = new JPanel();
                    getContentPane().add(jPanel1, BorderLayout.CENTER);
                    jPanel1.setLayout(null);
                    jPanel1.setBorder(BorderFactory.createBevelBorder(BevelBorder.RAISED));
                    {
                        localRadioButton = new JRadioButton();
                        jPanel1.add(localRadioButton);
                        localRadioButton.setText("Local");
                        localRadioButton.setBounds(20, 61, 65, 19);
                        localRadioButton.setActionCommand("local");
                        localRadioButton.addActionListener(this);
                    }
                    {
                        remoteRadioButton = new JRadioButton();
                        jPanel1.add(remoteRadioButton);
                        remoteRadioButton.setText("Remote");
                        remoteRadioButton.setBounds(20, 103, 65, 19);
                        remoteRadioButton.setActionCommand("remote");
                        remoteRadioButton.addActionListener(this);
                    }
                    {
                        ButtonGroup g = new ButtonGroup();
                        g.add(localRadioButton);
                        g.add(remoteRadioButton);
                    }
                    {
                        ComboBoxModel comComboBoxModel = new DefaultComboBoxModel(new String[] {});
                        comComboBox = new JComboBox();
                        jPanel1.add(comComboBox);
                        comComboBox.setModel(comComboBoxModel);
                        comComboBox.setBounds(89, 61, 71, 22);
                    }
                    {
                        ComboBoxModel baudComboBoxModel = new DefaultComboBoxModel(BAUDS);
                        baudComboBox = new JComboBox();
                        jPanel1.add(baudComboBox);
                        baudComboBox.setModel(baudComboBoxModel);
                        baudComboBox.setBounds(165, 61, 71, 22);
                    }
                    {
                        serverTextField = new JTextField();
                        jPanel1.add(serverTextField);
                        serverTextField.setText("127.0.0.1");
                        serverTextField.setBounds(89, 102, 91, 22);
                    }
                    {
                        portTextField = new JTextField();
                        jPanel1.add(portTextField);
                        portTextField.setText("9002");
                        portTextField.setBounds(184, 102, 34, 22);
                    }
                    {
                        jLabel1 = new JLabel();
                        jPanel1.add(jLabel1);
                        jLabel1.setText("Please select data source:");
                        jLabel1.setBounds(20, 25, 156, 15);
                    }
                }
                {
                    jPanel2 = new JPanel();
                    getContentPane().add(jPanel2, BorderLayout.SOUTH);
                    {
                        okButton = new JButton();
                        jPanel2.add(okButton);
                        okButton.setText("OK");
                        okButton.setPreferredSize(new Dimension(70,22));
                        okButton.setActionCommand("ok");
                        okButton.addActionListener(this);
                    }
                    {
                        cancelButton = new JButton();
                        jPanel2.add(cancelButton);
                        cancelButton.setText("Cancel");
                        cancelButton.setPreferredSize(new Dimension(70,22));
                        cancelButton.setActionCommand("cancel");
                        cancelButton.addActionListener(this);
                    }
                }
            }
        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    private void loadCOM() {
        comComboBox.removeAllItems();
        Enumeration e = CommPortIdentifier.getPortIdentifiers();
        while (e.hasMoreElements()) {
            CommPortIdentifier c = (CommPortIdentifier) e.nextElement();
            if (c.getPortType() == CommPortIdentifier.PORT_SERIAL)
                comComboBox.addItem(c.getName());
        }
    }

    private void setMode() {
        String comm = Config.get("comm-string");
        if (comm.startsWith("serial")) {
            setMode(MODE_LOCAL);
            comComboBox.setSelectedItem(comm.substring(comm.indexOf("@")+1, comm.indexOf(":")));
            baudComboBox.setSelectedItem(comm.substring(comm.indexOf(":")+1, comm.length()));
        } else {
            setMode(MODE_REMOTE);
            serverTextField.setText(comm.substring(comm.indexOf("@")+1, comm.indexOf(":")));
            portTextField.setText(comm.substring(comm.indexOf(":")+1, comm.length()));
        }
    }

    private void setMode(int mode) {
        localRadioButton.setSelected((mode == MODE_LOCAL));
        comComboBox.setEnabled((mode == MODE_LOCAL));
        baudComboBox.setEnabled((mode == MODE_LOCAL));

        remoteRadioButton.setSelected((mode == MODE_REMOTE));
        serverTextField.setEnabled((mode == MODE_REMOTE));
        portTextField.setEnabled((mode == MODE_REMOTE));
    }

    @Override
    public void actionPerformed(ActionEvent arg0) {
        String action = arg0.getActionCommand();
        if (action.equals("ok")) {
            String comm = (localRadioButton.isSelected()) ?
                ("serial@" + comComboBox.getSelectedItem() + ":" + baudComboBox.getSelectedItem()) :
                ("sf@" + serverTextField.getText() + ":" + portTextField.getText());
            Config.set("comm-string", comm, true);
            dispose();
        } else if (action.equals("cancel")) {
            dispose();
        } else if (action.equals("local")) {
            setMode(MODE_LOCAL);
        } else if (action.equals("remote")) {
            setMode(MODE_REMOTE);
        }
    }

}
