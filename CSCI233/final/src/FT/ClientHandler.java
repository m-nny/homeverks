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
//            sendMessage("Done");
            if (files.size() <= 0 || 5 < files.size()) {
                closeConnection("Illegal number of files");
                return;
            }
            FileTracker.registerFiles(files);
            while (true) {
                inMessage = readMessage();
                log("Client says:|" + inMessage + "|");
                if (inMessage.trim().length() == 0 || "END".equals(inMessage))
                    break;
                if (inMessage.startsWith("SEARCH: ")) {
                    outMessage = search(inMessage.substring(8));
                } else {
                    outMessage = inMessage;
                }
                sendMessage(outMessage);
            }
            closeConnection("Bye");
        } catch (Exception e) {
            e.printStackTrace();
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
        dOut.print(message + "\r\n");
        dOut.flush();
    }
    private void closeConnection(String message) throws IOException {
        sendMessage(message);
        this.clientSocket.close();
    }

    private void log(String message) {
        System.out.format("FT [%d]: %s\n", getClientID(), message);
    }

    private String search(String filename) {
        List<FileModel> list = FileTracker.map.get(filename);
        if (list == null || list.isEmpty())
            return "NOT FOUND";
        StringBuilder result = new StringBuilder("FOUND:");
        for (FileModel file: list) {
            result.append(file.toString());
        }
        return result.toString();
    }
}
