package Peer;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Random;
import javax.swing.DefaultListModel;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JScrollPane;
import javax.swing.JTextField;


public class MainGUI extends JFrame implements ActionListener {
    private JButton search;  //Buttons
    private JButton dLoad;
    private JButton close;

    private JList jl;   // List that will show found files
    private JLabel label; //Label "File Name
    private JTextField tf, tf2; // Two textfields: one is for typing a file name, the other is just to show the selected file
    private DefaultListModel listModel; // Used to select items in the list of found files

    private String str[] = {"Info1", "Info2", "Info3", "Info4", "Info5"}; // Files information

    private Peer peer;

    public MainGUI(Peer peer) {
        super("Example GUI");
        this.peer = peer;
        setLayout(null);
        setSize(500, 600);

        label = new JLabel("File name:");
        label.setBounds(50, 50, 80, 20);
        add(label);

        tf = new JTextField();
        tf.setBounds(130, 50, 220, 20);
        add(tf);

        search = new JButton("Search");
        search.setBounds(360, 50, 80, 20);
        search.addActionListener(this);
        add(search);

        listModel = new DefaultListModel();
        jl = new JList(listModel);

        JScrollPane listScroller = new JScrollPane(jl);
        listScroller.setBounds(50, 80, 300, 300);

        add(listScroller);

        dLoad = new JButton("Download");
        dLoad.setBounds(200, 400, 130, 20);
        dLoad.addActionListener(this);
        add(dLoad);

        tf2 = new JTextField();
        tf2.setBounds(200, 430, 130, 20);
        add(tf2);

        close = new JButton("Close");
        close.setBounds(360, 470, 80, 20);
        close.addActionListener(this);
        add(close);

        setVisible(true);
    }

    public void actionPerformed(ActionEvent e) {
        if (e.getSource() == search) { // If search button is pressed show 25 randomly generated file info in text area
            String fileName = tf.getText();
            List<String> result;
            try {
                result = peer.search(fileName.split("\\|")[0]);
            } catch (IOException e1) {
                e1.printStackTrace();
                return;
            }
            listModel.clear();
            if (result != null) {
                for (String line : result) {
                    listModel.addElement(line);
                }
            }
        } else if (e.getSource() == dLoad) {   // If download button is pressed get the selected value from the list and show it in text field
            try {
                peer.downloadFile(jl.getSelectedValue().toString().split("\\|")[0]);
            } catch (Exception e1) {
                e1.printStackTrace();
            }
            tf2.setText(jl.getSelectedValue().toString() + " downloaded or not");
        } else if (e.getSource() == close) { // If close button is pressed exit
            System.exit(0);
        }

    }

    public static void main(String[] args) throws Exception {
        String workingDir = args[0];
        String FTAddress = args[1];
        int FTPort = Integer.parseInt(args[2]);
        int PeerPort = Integer.parseInt(args[3]);
        Peer peer = new Peer(workingDir, PeerPort);
        peer.connect(FTAddress, FTPort);
        peer.registerOnFT();

        MainGUI ex = new MainGUI(peer);
        ex.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE); // Close the window if x button is pressed
    }
}