package FT;

import java.io.*;
import java.net.Socket;
import java.util.LinkedList;
import java.util.List;

public class ClientHandler implements Runnable {
    private Socket clientSocket;
    private static Integer lastClientID = 0;
    private int clientID = -1;
    private BufferedReader dIn;
    private PrintWriter dOut;

    ClientHandler(Socket client) {
        this.clientSocket = client;
    }

    private int getClientID() {
        if (this.clientID != -1)
            return this.clientID;
        synchronized (lastClientID) {
            clientID = lastClientID++;
        }
        return clientID;
    }

    @Override
    public void run() {
        try {
            log("Listening client");
            dIn = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            dOut = new PrintWriter(clientSocket.getOutputStream());

            String inMessage, outMessage;
            inMessage = readMessage();
            log("Client says:|" + inMessage + "|");
            if (!"HELLO".equals(inMessage)) {
                closeConnection("Illegal format");
                return;
            }
            sendMessage("HI");
            List<FileModel> files = new LinkedList<>();
            while (true) {
                inMessage = readMessage();
                log("Client says:|" + inMessage + "|");
                if (inMessage.trim().length() == 0 || "END".equals(inMessage))
                    break;
                try {
                    files.add(new FileModel(inMessage));
                } catch (Exception ex) {
                    System.out.println(ex.getMessage() + " Illegal format");
                    closeConnection("Illegal format");
                    return;
                }
            }
            sendMessage("DONE");
            if (files.size() <= 0 || 5 < files.size()) {
                closeConnection("Illegal number of files");
                return;
            }
            FileTracker.registerFiles(files);
            while (true) {
                inMessage = readMessage();
                log("Client says:|" + inMessage + "|");
                if (inMessage == null || inMessage.length() == 0 || "END".equals(inMessage))
                    break;
                if (inMessage.startsWith("SEARCH: ")) {
                    outMessage = search(inMessage.substring(8));
                } else if (inMessage.startsWith("SCORE of ")) {
                    handlePoints(inMessage);
                    outMessage = null;
                } else if (inMessage.equals("BYE")) {
                    break;
                } else {
                    outMessage = inMessage;
                }
                sendMessage(outMessage);
            }
            closeConnection("BYE");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void handlePoints(String message) throws IOException {
        message = message.substring(9);
        int last = message.lastIndexOf(':');
        String address = message.substring(0, last);
        String n = message.substring(last + 2);
        FileTracker.incNumberOfRequests(address);
        if ("0".equals(n)) {
        } else if ("1".equals(n)) {
            FileTracker.incNumberOfUploads(address);
        } else {
            closeConnection("Illegal message");
        }
    }

    private String readMessage() throws IOException {
        String msg = dIn.readLine();
        if (msg == null)
            return null;
        StringBuilder message = new StringBuilder(msg);
        int cc = message.length();
        if (cc > 0 && message.charAt(cc - 1) == '\r') {
            message.setLength(cc - 1);
            cc--;
        }
        if (cc > 0 && message.charAt(cc - 1) == '\n') {
            message.setLength(cc - 1);
            cc--;
        }
        if (cc > 0 && message.charAt(cc - 1) == '\r') {
            message.setLength(cc - 1);
            cc--;
        }
        return message.toString();
    }

    private void sendMessage(String message) {
        if (message == null)
            return;
        dOut.print(message + "\r\n");
        dOut.flush();
    }

    private void closeConnection(String message) throws IOException {
        FileTracker.cleanup(clientSocket.getInetAddress().getHostAddress());
        sendMessage(message);
        log(message);
        this.clientSocket.close();
    }

    private void log(String message) {
        System.out.format("FT [%d]: %s\n", getClientID(), message);
    }

    private String search(String filename) {
        List<FileModel> list = FileTracker.filesMap.get(filename);
        if (list == null || list.isEmpty())
            return "NOT FOUND";
        StringBuilder result = new StringBuilder("FOUND:");
        for (FileModel file : list) {
            String buf = file.toString() + ("|") + (FileTracker.getRate(file.address));
            System.out.println("Search result: " + buf);
            result.append(file.toString()).append("|").append(FileTracker.getRate(file.address));
        }
        return result.toString();
    }
}
