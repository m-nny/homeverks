package FT;

public class Main {
    public static void main(String[] args) {
        String address = "localhost";
        int port = 3397;
        FileTracker ft = new FileTracker(address, port);
        ft.start();
    }
}