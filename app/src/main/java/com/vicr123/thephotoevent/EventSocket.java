package com.vicr123.thephotoevent;

import android.content.Context;
import android.net.Network;
import android.os.Handler;
import android.support.annotation.Nullable;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.security.InvalidKeyException;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.LinkedBlockingQueue;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import static com.vicr123.thephotoevent.Globals.sock;

abstract class NetworkBlock {
    ByteArrayOutputStream data;
    String id;

    NetworkBlock(String id) {
        data = new ByteArrayOutputStream();
    }

    abstract void beforeSend();
    abstract void afterSend();
    abstract void progress(int bytesSent, int bytesTotal);
}

class NetworkWriteThread extends Thread {
    LinkedBlockingQueue<NetworkBlock> blocks;

    NetworkWriteThread() {
        blocks = new LinkedBlockingQueue<>();
    }

    public void run() {
        try {
            while (!isInterrupted() && blocks.remainingCapacity() != 0) {
                try {
                    NetworkBlock block = blocks.take();

                    block.beforeSend();

                    ByteBuffer buffer = ByteBuffer.wrap(block.data.toByteArray());

                    while (buffer.hasRemaining()) {
                        int readIn = 2048;
                        if (readIn > buffer.remaining()) readIn = buffer.remaining();

                        byte[] bytesToWrite = new byte[readIn];
                        buffer.get(bytesToWrite, 0, readIn);

                        sock.socket.getOutputStream().write(bytesToWrite);
                        sock.socket.getOutputStream().flush();

                        block.progress(buffer.position(), buffer.limit());
                    }

                    block.afterSend();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        } catch (InterruptedException e) {
            //Do nothing, time to quit!
        }

        try {
            sock.socket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}

public class EventSocket {
    NetworkWriteThread thread = null;
    Context ctx;
    Handler mainHandler;
    boolean authenticated = false;
    SSLSocket socket;

    Runnable onError, onEstablished, onClose, onEndOfPhotoQueue;

    public EventSocket(Context ctx) {
        super();
        this.ctx = ctx;
        mainHandler = new Handler(ctx.getMainLooper());

        thread = new NetworkWriteThread();
        thread.start();
    }

    public void writeImage(byte[] bytes, final SendImageCallback callback) {
        try {
            NetworkBlock block = new NetworkBlock("image") {
                @Override
                void beforeSend() {
                    System.out.println("Sending image...");
                    callback.beforeSend();
                }

                @Override
                void afterSend() {
                    System.out.println("Image sent");
                    callback.afterSend();
                }

                @Override
                void progress(int bytesSent, int bytesTotal) {
                    System.out.println("Image progress " + bytesSent + " of " + bytesTotal);
                    callback.progress(bytesSent, bytesTotal);
                }
            };

            block.data.write(("IMAGE START " + Integer.toString(bytes.length) + "\n").getBytes());
            block.data.write(bytes);

            thread.blocks.put(block);
        } catch (Exception e) {

        }
    }

    public void sendCommand(String command) {
        try {
            NetworkBlock block = new NetworkBlock("command") {
                @Override
                void beforeSend() {

                }

                @Override
                void afterSend() {

                }

                @Override
                void progress(int bytesSent, int bytesTotal) {

                }
            };
            block.data.write((command + "\n").getBytes());

            thread.blocks.put(block);
        } catch (Exception e) {

        }
    }

    public void setOnErrorHandler(Runnable r) {
        onError = r;
    }

    public void setOnEstablished(Runnable r) {
        onEstablished = r;
    }

    public void setOnClose(Runnable r) {
        onClose = r;
    }

    public void setOnEndOfPhotoQueue(Runnable r) {
        onEndOfPhotoQueue = r;
    }

    public void connect(SocketAddress endpoint, int timeout) throws IOException, KeyManagementException, NoSuchAlgorithmException {
        Socket tcpSocket = new Socket();
        tcpSocket.connect(endpoint, timeout);

        SSLContext sslContext = SSLContext.getInstance("TLS");
        sslContext.init(null, new TrustManager[] {
                new X509TrustManager() {
                    @Override
                    public void checkClientTrusted(X509Certificate[] x509Certificates, String s) throws CertificateException {
                        //Trust everyone
                    }

                    @Override
                    public void checkServerTrusted(X509Certificate[] x509Certificates, String s) throws CertificateException {
                        //Trust everyone
                    }

                    @Override
                    public X509Certificate[] getAcceptedIssuers() {
                        return new X509Certificate[0];
                    }
                }
        }, null);

        socket = (SSLSocket) sslContext.getSocketFactory().createSocket(tcpSocket, "localhost", 26157, true);

        Thread readThread = new Thread() {
            public void run() {
                try {
                    InputStream stream = socket.getInputStream();
                    while (this.isAlive()) {
                        byte[] buffer = new byte[2048];
                        int read = stream.read(buffer);

                        //Check if this is a handshake failure
                        if (!authenticated && new String(buffer, 0, read, "UTF-8").equals("HANDSHAKE FAIL\n")) {
                            //Error
                            mainHandler.post(onError);

                            thread.interrupt();
                            return;
                        }

                        //Decrypt the data

                        String messages = new String(buffer, "UTF-8").trim();
                        String[] messagesArray = messages.split("\n");

                        for (String message : messagesArray) {
                            if (message.equals("HANDSHAKE OK")) {
                                authenticated = true;
                                mainHandler.post(onEstablished);
                            } else if (message.equals("CLOSE")) {
                                //Finalize the connection
                                mainHandler.post(onClose);

                                thread.interrupt();
                            } else if (message.equals("ERROR")) {
                                //Close the connection
                                mainHandler.post(onClose);

                                thread.interrupt();
                            }
                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        };
        readThread.start();

        sock.sendCommand("HELLO");
    }

    public void close() {
        mainHandler.post(onClose);

        thread.interrupt();
    }
}
