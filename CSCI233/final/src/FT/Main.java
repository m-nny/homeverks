package FT;

public class Main {
    public static void main(String[] args) {
        String address = args[0];
        int port = Integer.parseInt(args[1]);
        FileTracker.init(address, port);
        FileTracker.start();
    }
}