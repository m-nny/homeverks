package m_nny.Server;

import java.net.Socket;
import java.util.List;
import java.util.Map;

public class ClientHandler implements Runnable {
    private Session currentClient;
    private Group currentGroup;
    private final List<Session> allClients;
    private Map<String, Group> allGroups;

    ClientHandler(Socket socket, List<Session> clientList, Map<String, Group> groupList) {
        this.currentClient = new Session(socket);
        this.currentGroup = null;
        this.allClients = clientList;
        this.allGroups = groupList;
    }

    @Override
    public void run() {
        String inputMessage;
        String outputMessage;
        boolean accepted = false;
        do {
            inputMessage = currentClient.read();
            if (inputMessage == null) {
                quit();
                return;
            }
            if (!inputMessage.startsWith("server hello ")) {
                outputMessage = "Illegal format. Try again.";
            } else {
                if (authorize(inputMessage.substring(13))) {
                    accepted = true;
                    outputMessage = "hi " + currentClient.getUsername();
                } else {
                    outputMessage = "Username is already taken. Try again.";
                }
            }
            currentClient.write(outputMessage, !accepted);
        } while (!accepted);

        System.out.println("Welcome, " + currentClient.getUsername());
        this.joinGroup("all");

        for (;;) {
            String line = currentClient.read();
            if (line == null) {
                quit();
                return;
            }
            handleMessage(line);
        }
    }

    private void handleMessage(String msg) {
        if (msg.equals("server groupslist")) {
            getGroupList();
            return;
        }
        if (msg.startsWith("server join ")) {
            joinGroup(msg.substring(12));
            return;
        }
        if (msg.startsWith("server members")) {
            groupMembers();
            return;
        }
        if (msg.equals("server leave")){
            leaveGroup();
            return;
        }
        if (msg.equals("server exit")) {
            this.quit();
        }
        if (msg.startsWith("toall ")) {
            sendMessage(msg.substring(6));
            return;
        }
        int ind = msg.indexOf(' ');
        if (ind != -1) {
            sendPrivateMessage(msg.substring(0, ind), msg.substring(ind + 1));
        } else {
            currentClient.writeError("Illegal command. Try again.");
        }
    }

    private void leaveGroup() {
        if (currentGroup == null) {
            currentClient.writeError("You are already leaved your group.");
            return;
        }
        broadcast(currentClient.getUsername() + " has left this group.");
        currentGroup.removeMember(currentClient);
        currentGroup = null;
    }

    private void joinGroup(String name) {
        if (currentGroup != null) {
            if (currentGroup.getName().equals(name)) {
                currentClient.writeError("You are already in this group.");
                return;
            } else {
                currentClient.writeError("You are already in group. Leave previous group first");
                return;
            }
        }
        Group destination = allGroups.get(name);
        if (destination == null) {
            currentClient.writeError("There is no such group. Use \"server groupslist\" to get list of available groups");
            return;
        }
        currentGroup = destination;
        currentGroup.addMember(currentClient);
    }

    private void groupMembers() {
        if (currentGroup == null) {
            currentClient.writeError("You are not member of any group.");
            return;
        }
        StringBuilder result = new StringBuilder();
        for (Session client: currentGroup.getMembers()) {
            result.append(client.getUsername()).append(", ");
        }
        if (!currentGroup.getMembers().isEmpty()) {
            result.setLength(result.length() - 2);
        }
        currentClient.writeOK(result.toString());
    }

    private void getGroupList() {
        StringBuilder result = new StringBuilder();
        for (Map.Entry<String, Group> entry: allGroups.entrySet()) {
            result.append(entry.getValue()).append(" | ");
        }
        if (!allGroups.isEmpty()) {
            result.setLength(result.length() - 3);
        }
        currentClient.writeOK(result.toString());
    }

    private boolean authorize(String username) {
        synchronized (allClients) {
            for (Session client : allClients) {
                if (username.equals(client.getUsername()))
                    return false;
            }
            currentClient.setUsername(username);
            allClients.add(currentClient);
        }
        return true;
    }

    private void sendMessage(String msg) {
        msg = currentClient.getUsername() + ": " + msg;
        broadcast(msg);
    }

    private void sendPrivateMessage(String receiver, String msg) {
        msg = currentClient.getUsername() + ": " + msg;
        for (Session client : allClients) {
            if (receiver.equals(client.getUsername())) {
                client.writeOK(msg);
                return;
            }
        }
        currentClient.writeError("There is no such user");
    }

    private void broadcast(String msg) {
        if (currentGroup == null) {
            currentClient.writeError("You are not member of any group.");
            return;
        }
        List<Session> receivers = currentGroup.getMembers();
        for (Session client : receivers) {
//            if (client != currentClient)
                client.writeOK(msg);
        }
    }

    private void quit() {
        String username = currentClient.getUsername();
        if (username == null)
            username = "Unknown";
        System.out.println(username + " has gone");
        allClients.remove(currentClient);
        currentClient.close();
    }
}
