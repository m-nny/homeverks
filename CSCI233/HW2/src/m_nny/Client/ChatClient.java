package m_nny.Client;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.Scanner;

class ChatClient {
    private Socket socket;
    private BufferedReader inputSocket;
    private Scanner inputKeyboard;
    private PrintWriter outputSocket;

    ChatClient(String address, int port) throws Exception {
        socket = new Socket(address, port);
        try {
            inputSocket = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            outputSocket = new PrintWriter(socket.getOutputStream());
            inputKeyboard = new Scanner(System.in);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void start() {
        System.out.println("Connected to server");
        new Thread(this::waitForKeyboard).start();
        new Thread(this::waitForSocket).start();
    }

    private void waitForKeyboard() {
        for (; ; ) {
            String line = inputKeyboard.nextLine();
            writeSocket(line);
        }
    }

    private void waitForSocket() {
        for (; ; ) {
            String line = readSocket();
            if (line == null) {
                System.out.println("Server closed connection");
                System.exit(0);
            }
            System.out.println(line);
        }
    }

    private String readSocket() {
        String line;
        try {
            line = inputSocket.readLine();
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
        return line;
    }

    private void writeSocket(String line) {
        if (!socket.isConnected()) {
            System.out.println("Server closed connection");
            System.exit(0);
        }
        outputSocket.println(line);
        outputSocket.flush();
    }
}
