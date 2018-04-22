package Peer;

import FT.FileModel;

import java.io.*;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

class Peer {
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
        Socket ftSocket = new Socket(address, port);
        dIn = new BufferedReader(new InputStreamReader(ftSocket.getInputStream()));
        dOut = new PrintWriter(ftSocket.getOutputStream());
        new Thread(this::peerAccept).start();
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
        System.out.println("FT says: |" + inMessage + "|");
        if (inMessage == null || inMessage.isEmpty() || "NOT FOUND".equals(inMessage))
            return null;
        List<String> list = new LinkedList<>();
        Pattern pattern = Pattern.compile("(<[^>]+>)\\|(\\d+%)");
        Matcher matcher = pattern.matcher(inMessage);
        while (matcher.find()) {
            list.add(matcher.group(1) + "|" + matcher.group(2));
        }
        return list;
    }

    void downloadFile(String string) throws Exception {

        FileModel fm = new FileModel(string);
        Socket sock = new Socket(fm.address, fm.port);
        InputStream in = sock.getInputStream();
        PrintWriter out = new PrintWriter(sock.getOutputStream());
        FileOutputStream file = new FileOutputStream(folder.getAbsolutePath() + File.separator + fm.fileName + "." + fm.fileType);
        out.println("DOWNLOAD:" + fm.fileName + "," + fm.fileType + "," + fm.fileSize);
        out.flush();
        byte[] status = new byte[4];
        byte[] buffer = new byte[4096];
        int fbytes = in.read(status, 0, 4);
        int done = 0;
        if (fbytes == 4 && new String(status).equals("YES!")) {
            while ((fbytes = in.read(buffer)) > 0) {
//                System.out.format("peer sent:|%s|", new String(buffer).substring(20));
                file.write(buffer, 0, fbytes);
                file.flush();
            }
            file.close();
            sock.close();
            done = 1;
        } else {
            System.out.println("No file received");
        }
        System.out.println("SCORE of " + sock.getInetAddress().getHostAddress() + ": " + done);
        dOut.println("SCORE of " + sock.getInetAddress().getHostAddress() + ": " + done);
        dOut.flush();

    }

    private void uploadFile(Socket sock) throws IOException {
        BufferedReader in = new BufferedReader(new InputStreamReader(sock.getInputStream()));
        OutputStream out = sock.getOutputStream();
        String reqFile = in.readLine();
        String fName = reqFile.split("[:,]")[1];
        String fType = reqFile.split("[:,]")[2];
//        int fSize = Integer.parseInt(reqFile.split(":|,")[3]);
        System.out.println("Client requests: |" + reqFile + "|");
        FileInputStream file = new FileInputStream(folder.getAbsolutePath() + File.separator + fName + "." + fType);
        byte[] buf = new byte[4098];
        int fbytes;
        Random i = new Random();
        if (i.nextInt(100) < 50) {
            out.write(("YES!").getBytes());
            while ((fbytes = file.read(buf)) > 0) {
//                System.out.format("file contained:|%s|", new String(buf).substring(20));
                out.write(buf, 0, fbytes);
                out.flush();
            }
        } else {
            out.write("NO!!".getBytes());
        }
        out.flush();
        file.close();
    }

    @SuppressWarnings("InfiniteLoopStatement")
    private void peerAccept() {
        try {
            ServerSocket serverSocket = new ServerSocket(peerPort);

            while (true) {
                Socket sock = serverSocket.accept();
                uploadFile(sock);
                sock.close();
            }


        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    void closeConnection() {
        dOut.println("BYE");
        dOut.flush();
    }
}
