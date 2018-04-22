package FT;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

class FileTracker {
    static private String address;
    static private int port;
    static final private Queue<Socket> clientsQueue = new LinkedList<>();
    static final private List<FileModel> allFiles = new LinkedList<>();
    static final public HashMap<String, List<FileModel>> map = new HashMap<>();

    static void init(String address, int port) {
        FileTracker.address = address;
        FileTracker.port = port;
    }

    static void start() {
        new Thread(FileTracker::waitForClients).start();
        new Thread(FileTracker::handleClients).start();
        System.out.format("FT: File Tracker has started on %s:%d\n", address, port);
    }

    static private void waitForClients() {
        try {
            ServerSocket serverSocket = new ServerSocket(port);
            System.out.println("FT: Waiting for clients to arrive");
            while (true) {
                Socket clientSocket = serverSocket.accept();
                synchronized (clientsQueue) {
                    clientsQueue.add(clientSocket);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    static private void handleClients() {
        while (true) {
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            synchronized (clientsQueue) {
                if (clientsQueue.isEmpty())
                    continue;
                Socket socket = clientsQueue.poll();
                new Thread(new ClientHandler(socket)).start();
            }
        }
    }

    static void registerFiles(List<FileModel> files) {
        synchronized (allFiles) {
            System.out.format("FT: new files came\n");
            allFiles.addAll(files);
            for(FileModel file: files) {
                if (!map.containsKey(file.fileName))
                    map.put(file.fileName, new LinkedList<>());
                List<FileModel> list = map.get(file.fileName);
                list.add(file);
            }
        }
    }

}
