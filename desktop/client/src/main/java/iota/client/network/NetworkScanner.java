package iota.client.network;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.*;

/**
 * Scans all IP 192.168.0.*:<port> addresses on the network for being open or not
 * Also spawns and kills 255 threads in the process, TODO: change that
 */
public class NetworkScanner {

    private String baseIp;

    public List<ScanResult> scan(int port) {
        String baseIp = "192.168.0.";


        try {
            byte[] ipv4 = InetAddress.getLocalHost().getAddress();
            baseIp = Byte.toUnsignedInt(ipv4[0]) + "." +
                    Byte.toUnsignedInt(ipv4[1]) + "." +
                    Byte.toUnsignedInt(ipv4[2]) + ".";
            System.out.println();
        } catch (UnknownHostException e) {
            System.out.println("Couldn't get a local IP, using fallback of 192.168.0.*");
        }

        System.out.println("Beginning scan of " + baseIp + "*");
        final ExecutorService es = Executors.newFixedThreadPool(255);
        final int timeout = 1000;
        List<Future<ScanResult>> futures = new ArrayList<>();

        for (int i = 0; i <= 255; i++) {
            futures.add(portIsOpen(es, (baseIp + i), port, timeout));
        }
        es.shutdown();
        ArrayList<ScanResult> results = new ArrayList<>();
        boolean allDone = false;
        while (!allDone) {
            allDone = true;
            for (int i = 0; i < futures.size(); i++) {
                if (!futures.get(i).isDone()) {
                    allDone = false;
                }
            }
        }
        for (int i = 0; i < futures.size(); i++) {
            try {
                results.add(futures.get(i).get());
            } catch (InterruptedException e) {
                e.printStackTrace();
            } catch (ExecutionException e) {
                e.printStackTrace();
            }
        }
        System.out.println("Finished Scan");
        return results;
    }

    private static Future<ScanResult> portIsOpen(final ExecutorService es, final String ip, final int port, final int timeout) {
        return es.submit(new Callable<ScanResult>() {
            @Override
            public ScanResult call() {
                try (Socket socket = new Socket()) {
                    socket.connect(new InetSocketAddress(ip, port), timeout);
                    socket.close();
                    return new ScanResult(ip, port, true);
                } catch (Exception ex) {
                    return new ScanResult(ip, port, false);
                }
            }
        });
    }
}