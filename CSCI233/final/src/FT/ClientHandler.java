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

            String message = readMessage();
            log("Client says:|" + message + "|");
            if (!"HELLO".equals(message)) {
                closeConnection("Illegal format");
                return;
            }
            sendMessage("HI");
            List<FileModel> files = new LinkedList<>();
            while (true) {
                message = readMessage();
                log("Client says:|" + message + "|");
                if (message.trim().length() == 0 || "END".equals(message))
                    break;
                try {
                    files.add(new FileModel(message));
                } catch (Exception ex) {
                    System.out.println(ex.getMessage() + " Illegal format");
                    closeConnection("Illegal format");
                    return;
                }
                FileTracker.registerFiles(files);
            }
            sendMessage("Done");
            if (files.size() <= 0 || 5 < files.size()) {
                closeConnection("Illegal number of files");
                return;
            }
            while (true) {
                message = readMessage();
                log("Client says:|" + message + "|");
                if (message.trim().length() == 0 || "END".equals(message))
                    break;
                sendMessage(message);
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
}
