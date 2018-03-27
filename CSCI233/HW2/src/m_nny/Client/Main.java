package m_nny.Client;

public class Main {
    public static void main(String[] args) throws Exception {
        if (args.length < 2) {
            System.err.println("Pass server address and port to connect");
            System.exit(0);
        }
        String address = args[0];
        int port = Integer.parseInt(args[1]);
        ChatClient client = new ChatClient(address, port);
        client.start();
    }

}
