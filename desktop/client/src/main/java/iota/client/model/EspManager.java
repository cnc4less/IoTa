package iota.client.model;

import iota.client.network.ConnectionStatus;
import iota.client.network.NetworkScanner;
import iota.client.network.ScanResult;
import iota.common.definitions.DefinitionStore;

import java.io.IOException;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Observable;
import java.util.concurrent.ConcurrentHashMap;

public class EspManager extends Observable implements Runnable {
    private Thread t;
    private String threadName;
    private ConcurrentHashMap<InetAddress, EspDevice> devices;
    private DefinitionStore definitionStore;

    private volatile boolean running = true;

    public EspManager(DefinitionStore definitionStoreIn) {
        devices = new ConcurrentHashMap<>();
        this.threadName = this.toString();
        this.definitionStore = definitionStoreIn;

    }

    public Map<InetAddress, EspDevice> getDevMap() {
        return devices;
    }

    @Override
    public void run() {
        while (running) {
            updateMap();
            try {
                Thread.sleep(5000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void start() {
        System.out.println("Starting " + threadName);
        if (t == null) {
            t = new Thread(this, threadName);
            t.start();
        }
    }

    private void updateMap() {

        ArrayList<EspDevice> staging = new ArrayList<>();

        NetworkScanner lanScan = new NetworkScanner();
        List<ScanResult> results = lanScan.scan(2812);
        for (ScanResult r : results) {
            if (r.isOpen()) {
                if (!devices.containsKey(r.getHost())) {
                    EspDevice device = new EspDevice(r.getHost(), definitionStore);
                    staging.add(device);
                }
            }
        }

        for (EspDevice d : staging) {
            try {
                d.start();
            } catch (IOException e) {
                System.out.println("exception, couldn't intialise");
                e.printStackTrace();
            }
        }

        try {
            //timeout length
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }


        for (EspDevice d : staging) {
            if (d.getStatus() == ConnectionStatus.CONNECTED) {
                System.out.println("Client at " + d.getInetAddress() + " has been authenticated, adding to store");
                devices.put(d.getInetAddress(), d);
            } else {
                System.out.println(d.getInetAddress() + " timeout while attempting to connect");
            }
        }


        setChanged();
        notifyObservers();
    }
}
