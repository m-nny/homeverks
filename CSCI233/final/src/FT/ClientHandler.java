package FT;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.LinkedList;
import java.util.List;

public class ClientHandler implements Runnable {
    private Socket clientSocket;
    private static Integer lastClientID = 0;
    private int clientID = -1;
    private DataInputStream dIn;
    private DataOutputStream dOut;

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
            dIn = new DataInputStream(clientSocket.getInputStream());
            dOut = new DataOutputStream(clientSocket.getOutputStream());

            String message = dIn.readLine();
            log("Client says:|" + message + "|");
            if (!"HELLO".equals(message)) {
                closeConnection("Illegal format");
                return;
            }
            dOut.writeChars("HI");
            dOut.flush();
            List<FileModel> files = new LinkedList<>();
            while (true) {
                message = dIn.readLine();
                log("Client says:|" + message + "|");
                if (message.isEmpty() || "END".equals(message))
                    break;
                files.add(new FileModel(message));
                FileTracker.registerFiles(files);
            }
            if (files.size() <= 0|| 5 < files.size()) {
                closeConnection("Illegal number of files");
                return;
            }
            while (true) {
                message = dIn.readLine();
                log("Client says:|" + message + "|");
                if (message.isEmpty())
                    break;
                dOut.writeChars(message);
                dOut.flush();
            }
            closeConnection("Bye");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void closeConnection(String message) throws IOException {
        dOut.writeChars(message);
        this.clientSocket.close();
    }

    private void log(String message) {
        System.out.format("FT [%d]: %s\n", getClientID(), message);
    }
}
