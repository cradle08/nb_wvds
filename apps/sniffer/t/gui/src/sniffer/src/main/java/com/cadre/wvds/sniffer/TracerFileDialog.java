package com.cadre.wvds.sniffer;

import javax.swing.JDialog;
import javax.swing.JPanel;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JTextField;

public class TracerFileDialog extends JDialog {

    private JTextField textField;
    private JTextField textField_1;

    public TracerFileDialog(JDialog parent) {
        super(parent, true);

        setTitle("TracerFile");
        setSize(400, 240);
        setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);

        getContentPane().setLayout(new BorderLayout(0, 0));

        JPanel panel = new JPanel();
        getContentPane().add(panel);
        panel.setLayout(null);

        JButton btnNewButton_2 = new JButton("...");
        btnNewButton_2.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                File file1 = new File(".");
                JFileChooser chooser = new JFileChooser(file1);
                int returnVal = chooser.showOpenDialog(null);
                if (returnVal == JFileChooser.APPROVE_OPTION) {
                    String fname = chooser.getSelectedFile().getPath();
                    textField_1.setText(fname);
                }
            }
        });
        btnNewButton_2.setBounds(362, 86, 24, 23);
        panel.add(btnNewButton_2);

        textField_1 = new JTextField();
        textField_1.setBounds(80, 87, 280, 21);
        panel.add(textField_1);
        textField_1.setColumns(10);

        JLabel lblNewLabel_1 = new JLabel("File path");
        lblNewLabel_1.setBounds(20, 90, 60, 15);
        panel.add(lblNewLabel_1);

        textField = new JTextField();
        textField.setBounds(80, 51, 60, 21);
        panel.add(textField);
        textField.setColumns(10);

        JLabel lblNewLabel = new JLabel("File id");
        lblNewLabel.setBounds(20, 54, 60, 15);
        panel.add(lblNewLabel);

        JPanel panel_1 = new JPanel();
        getContentPane().add(panel_1, BorderLayout.SOUTH);

        JButton btnNewButton = new JButton("OK");
        panel_1.add(btnNewButton);
        btnNewButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                TraceFile f = new TraceFile();
                f.setFid(Integer.parseInt(textField.getText()));
                f.setPath(textField_1.getText());
                Main.tracemgr.addFile(f);
                Main.tracemgr.save();
                dispose();
            }
        });

        JButton btnNewButton_1 = new JButton("Cancel");
        panel_1.add(btnNewButton_1);
        btnNewButton_1.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                dispose();
            }
        });
    }

}
