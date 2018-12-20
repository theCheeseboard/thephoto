/*
 *     thePhoto Event Mode Client: Sends photos to thePhoto on a PC
 *     Copyright (C) 2018 Victor Tran
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

package com.vicr123.thephotoevent;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.preference.PreferenceManager;
import androidx.annotation.Nullable;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.LinkedBlockingQueue;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
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
                NetworkBlock block = blocks.take();
                try {
                    block.beforeSend();

                    ByteBuffer buffer = ByteBuffer.wrap(block.data.toByteArray());

                    while (buffer.hasRemaining() && sock.socket != null) {
                        int readIn = 2048;
                        if (readIn > buffer.remaining()) readIn = buffer.remaining();

                        byte[] bytesToWrite = new byte[readIn];
                        buffer.get(bytesToWrite, 0, readIn);

                        sock.socket.getOutputStream().write(bytesToWrite);
                        sock.socket.getOutputStream().flush();

                        block.progress(buffer.position(), buffer.limit());
                    }

                    if (sock.socket != null) {
                        block.afterSend();
                    }
                } catch (IOException e) {
                    e.printStackTrace();

                    block.afterSend();
                }

                if (sock.socket == null) {
                    //We're disconnected; clear all the remaining images
                    blocks.clear();
                }
            }
        } catch (InterruptedException e) {
            //Do nothing, time to quit!
        }

        if (sock.socket != null) {
            try {
                sock.socket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}

public class EventSocket {
    NetworkWriteThread thread = null;
    Context ctx;
    Handler mainHandler;
    boolean authenticated = false;
    private boolean reconnecting = false;
    SSLSocket socket = null;
    long lastPingReceived = 0;
    Timer pingChecker;
    boolean closeRequested = false;

    private Runnable onError, onEstablished, onClose, onEndOfPhotoQueue, onUnexpectedDisconnect, onReconnect;

    public EventSocket(Context ctx) {
        super();
        this.ctx = ctx;
        mainHandler = new Handler(ctx.getMainLooper());

        thread = new NetworkWriteThread();
        thread.start();
    }

    public void writeImage(byte[] bytes, final SendImageCallback callback) {
        if (socket == null) return; //Do nothing if the socket is invalid
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
        if (socket == null) return; //Do nothing if the socket is invalid
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

    public void setOnUnexpectedDisconnect(Runnable r) {
        onUnexpectedDisconnect = r;
    }

    public void setOnReconnect(Runnable r) {
        onReconnect = r;
    }

    public void setOnEndOfPhotoQueue(Runnable r) {
        onEndOfPhotoQueue = r;
    }

    public boolean isReconnecting() {
        return reconnecting;
    }

    public void connect(final SocketAddress endpoint, final int timeout, final boolean isReconnect) throws IOException, KeyManagementException, NoSuchAlgorithmException {
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
                    while (socket != null && this.isAlive()) {
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

                                if (isReconnect) {
                                    reconnecting = false;
                                    mainHandler.post(onReconnect);
                                } else {
                                    mainHandler.post(onEstablished);
                                }

                                //Identify
                                SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(ctx);
                                String displayName = pref.getString("display_name", "");
                                if (displayName.equals("device_name")) {
                                    displayName = Globals.getDeviceName();
                                }

                                sock.sendCommand("IDENTIFY " + displayName);

                                //Register for pings
                                sock.sendCommand("REGISTERPING");
                            } else if (message.equals("CLOSE")) {
                                //Finalize the connection
                                mainHandler.post(onClose);

                                thread.interrupt();
                            } else if (message.equals("ERROR")) {
                                //Close the connection
                                mainHandler.post(onClose);

                                thread.interrupt();
                            } else if (message.equals("PING")) {
                                //Reply with a ping
                                sock.sendCommand("PING");
                                lastPingReceived = System.currentTimeMillis();

                                if (pingChecker == null) {
                                    pingChecker = new Timer("pingchecker");
                                    pingChecker.scheduleAtFixedRate(new TimerTask() {
                                        @Override
                                        public void run() {
                                            if (System.currentTimeMillis() - lastPingReceived > 30000) {
                                                //Set socket to null to tell everything we're gone
                                                Log.w("thePhoto", "Unexpectedly disconnected!");
                                                socket = null;
                                                authenticated = false;
                                                lastPingReceived = 0;
                                                mainHandler.post(onUnexpectedDisconnect);
                                                pingChecker.cancel();
                                                pingChecker = null;
                                                reconnecting = true;

                                                //Prepare to reconnect
                                                setOnErrorHandler(new Runnable() {
                                                    @Override
                                                    public void run() {
                                                        //Error occurred, just close
                                                        close();
                                                    }
                                                });

                                                //Attempt to reconnect after 1000 milliseconds
                                                new Timer().schedule(new TimerTask() {
                                                    @Override
                                                    public void run() {
                                                        Log.w("thePhoto", "Attempting a reconnect");
                                                        try {
                                                            connect(endpoint, timeout, true);
                                                        } catch (Exception e) {
                                                            //Error occurred; just close
                                                            close();
                                                        }
                                                    }
                                                }, 1000);
                                            }
                                        }
                                    }, 5000, 5000);
                                }
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

        //Time out after 5 seconds
        mainHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (!authenticated) {
                    //Time out
                    mainHandler.post(onError);
                    thread.interrupt();
                    close();
                }
            }
        }, 5000);
    }

    public void close() {
        if (!closeRequested) {
            closeRequested = true;
            sendCommand("CLOSE");

            mainHandler.post(onClose);
            thread.interrupt();

            if (pingChecker != null) {
                //Stop the timer
                pingChecker.cancel();
            }
        }
    }
}
