package m_nny.Server;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

class ChatServer {

    private ServerSocket server;
    private List<Session> clientList;
    private Map<String, Group> groupList;

    ChatServer(int port) {
        try {
            server = new ServerSocket(port);
            clientList = new LinkedList<>();
            groupList = new HashMap<>();
            this.loadDefaultGroups();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    protected void start() {
        System.out.println("ChatServer starting up on port " + server.getLocalPort());
        System.out.println("(press ctrl-c to exit)");

        System.out.println("Waiting for connection");
        for (; ; ) {
            try {
                Socket remote = server.accept();
                System.out.println("New client arrived");
                new Thread(new ClientHandler(remote, clientList, groupList)).start();

            } catch (Exception e) {
                System.out.println("Error: " + e);
            }
        }
    }

    private void loadDefaultGroups() {
        String[] defaultGroupNames = {"all", "thrash", "spam"};
        for (String name: defaultGroupNames)
            groupList.put(name, new Group(name));
    }
}
