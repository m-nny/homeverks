package m_nny.Server;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

class Session {
    private Socket sock;
    private String username;
    private BufferedReader input;
    private PrintWriter output;

    Session(Socket socket) {
        sock = socket;
        try {
            input = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            output = new PrintWriter(socket.getOutputStream());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    void writeOK(String str) {
        write(str, false);
    }

    void writeError(String str) {
        write(str, true);
    }

    void write(String str, boolean error) {
        str = (error ? "ERROR. " : "OK. ") + str;
        write(str);
    }

    private void write(String str) {
        output.println(str);
        output.flush();
    }

    String read() {
        if (isGone()) {
            System.out.println("Client is gone");
            return null;
        }
        String line = null;
        try {
            line = input.readLine();
        } catch (IOException e) {
            System.err.println(e);
            e.printStackTrace();
        }
        return line;
    }

    private boolean isGone() {
        return sock.isClosed();
    }

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public void close() {
        try {
            this.sock.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
