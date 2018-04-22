package Peer;

import FT.FileModel;

import java.io.*;
import java.net.InetAddress;
import java.net.Socket;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

class Peer {
    private Socket ftSocket;
    private int peerPort;
    private File folder;
    private BufferedReader dIn;
    private PrintWriter dOut;

    Peer(String dir, int peerPort) throws Exception {
        folder = new File(dir);
        if (!folder.isDirectory())
            throw new Exception("Not valid folder to share");
        this.peerPort = peerPort;
    }

    void connect(String address, int port) throws IOException {
        ftSocket = new Socket(address, port);
        dIn = new BufferedReader(new InputStreamReader(ftSocket.getInputStream()));
        dOut = new PrintWriter(ftSocket.getOutputStream());
        System.out.println("Client connect to server");
    }

    void registerOnFT() throws Exception {
        String inMessage, outMessage;

        dOut.println("HELLO");
        dOut.flush();
        inMessage = dIn.readLine();
        System.out.println("FT says: |" + inMessage + "|");

        File[] fileNames = folder.listFiles();
        if (fileNames == null) {
            throw new Exception("There are no such directory");
        }
        for (File file : fileNames) {
            outMessage = new FileModel(file, InetAddress.getLocalHost().getHostAddress(), peerPort).toString();
            dOut.println(outMessage);
        }
        dOut.println("END");
        dOut.flush();
    }

    List<String> search(String filename) throws IOException {
        String inMessage, outMessage;
        outMessage = "SEARCH: " + filename;
        dOut.println(outMessage);
        dOut.flush();
        inMessage = dIn.readLine();
        if (inMessage == null || inMessage.isEmpty() || "NOT FOUND".equals(inMessage))
            return null;
        List<String> list = new LinkedList<>();
        Pattern pattern = Pattern.compile("<([^>]+)>");
        Matcher matcher = pattern.matcher(inMessage);
        while (matcher.find()) {
            list.add(matcher.group(1));
        }
        return list;
    }

}
