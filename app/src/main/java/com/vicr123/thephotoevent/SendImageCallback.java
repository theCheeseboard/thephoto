package com.vicr123.thephotoevent;

public interface SendImageCallback {
    void beforeSend();
    void afterSend();
    void progress(int bytesSent, int bytesTotal);
}
