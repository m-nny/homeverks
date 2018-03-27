package m_nny.Server;

public class Main {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Pass port number");
            return;
        }
        int port = Integer.parseInt(args[0]);
        ChatServer wb = new ChatServer(port);
        wb.start();
    }
}
