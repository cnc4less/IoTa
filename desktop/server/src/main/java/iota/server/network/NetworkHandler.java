package iota.server.network;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NetworkHandler implements Runnable {

    private ServerSocket sock;
    private volatile boolean stop = false;
    private ExecutorService es;

    public NetworkHandler(int portNum) {
        try {
            sock = new ServerSocket(portNum);
            es = Executors.newFixedThreadPool(100);
        } catch (Exception e) {
            System.out.println(e.toString());
            System.out.println("Couldn't start iota.client.network handler on port " + portNum);
        }

    }

    public void run() {
        while (!stop) {
            try {
                Socket client = sock.accept();
                // Thread t = new Thread(new TcpDecoder(client));
                // t.start();
                es.execute(new TcpDecoder(client));
            } catch (IOException e) {
                System.err.println(e.getMessage());
            }

        }

    }

    public synchronized void requestStop() {
        this.stop = true;
    }
}
