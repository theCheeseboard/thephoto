package com.the.thephoto;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.hardware.Camera;
import android.media.AudioManager;
import android.media.MediaActionSound;
import android.media.MediaPlayer;
import android.os.AsyncTask;
import android.provider.MediaStore;
import android.support.design.widget.Snackbar;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.animation.Animation;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Created by victo on 26/12/2015.
 */
public class CameraView extends SurfaceView implements SurfaceHolder.Callback {
    interface onPictureTakenListener {
        void OnPictureTaken(byte[] data);
    }

    SurfaceHolder holder;
    Camera camera;
    Camera.Parameters params;
    Context cx;
    onPictureTakenListener onPictureTaken;
    float focusX, focusY;
    Paint p = new Paint();
    Paint bp = new Paint();
    int id = Camera.CameraInfo.CAMERA_FACING_BACK;

    Camera.AutoFocusCallback focusListener = new Camera.AutoFocusCallback() {
        @Override
        public void onAutoFocus(boolean success, Camera camera) {
            if (success) {
                new MediaActionSound().play(MediaActionSound.FOCUS_COMPLETE);
                p.setColor(Color.GREEN);
            } else {
                p.setColor(Color.RED);
            }
            postInvalidate();
        }
    };

    int Timer = 0;

    public void SwitchCamera() {
        releaseCamera();
        params = null;
        if (id == Camera.CameraInfo.CAMERA_FACING_BACK) {
            id = Camera.CameraInfo.CAMERA_FACING_FRONT;
        } else {
            id = Camera.CameraInfo.CAMERA_FACING_BACK;
        }
        openCamera();
    }

    public CameraView(Context context) {
        super(context);
        cx = context;

        openCamera();
        holder = getHolder();
        holder.addCallback(this);

        p.setColor(Color.WHITE);
        bp.setColor(Color.BLACK);
    }



    public void openCamera() {
        camera = Camera.open(id);
        if (params == null) {
            params = camera.getParameters();
            params.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
        }
        camera.setParameters(params);
        camera.setDisplayOrientation(90);

    }

    public void releaseCamera() {
        try {
            camera.setPreviewDisplay(null);
        } catch (IOException e) {
            e.printStackTrace();
        }
        camera.release();
    }

    public void capture() {
        MediaPlayer mp;
        switch (Timer) {
            case 0:
                captureImage();
                break;
            case 1:
                mp = MediaPlayer.create(cx, R.raw.timer_10_sec);
                mp.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
                    @Override
                    public void onCompletion(MediaPlayer mp) {
                        captureImage();
                    }
                });
                mp.start();
                break;
            case 2:
                mp = MediaPlayer.create(cx, R.raw.timer_3_sec);
                mp.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
                    @Override
                    public void onCompletion(MediaPlayer mp) {
                        captureImage();
                    }
                });
                mp.start();
                break;
        }
    }

    private void captureImage() {
        camera.takePicture(null, null, new Camera.PictureCallback() {
            @Override
            public void onPictureTaken(final byte[] data, Camera camera) {
                camera.stopPreview();
                new MediaActionSound().play(MediaActionSound.SHUTTER_CLICK);
                onPictureTaken.OnPictureTaken(data);

                AsyncTask writeTask = new AsyncTask() {
                    @Override
                    protected Object doInBackground(Object[] params) {
                        try {
                            OutputStream stream = vars.PcSock.getOutputStream();
                            stream.write(data);
                            stream.write("\r\n".getBytes("UTF-8"));
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        return 0;
                    }
                };
                writeTask.execute();
            }
        });
    }

    public void startPreview() {
        camera.startPreview();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        setWillNotDraw(false);
        try {
            camera.setPreviewDisplay(holder);
            camera.startPreview();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (RuntimeException e) {
        }

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public Camera getCamera() {
        return camera;
    }

    public void setRotation(int rotation) {
        try {
            params.setRotation(rotation);
            camera.setParameters(params);
        } catch (Exception e) {
            //Ignore
        }
    }

    public void setFlash(String type) {
        params.setFlashMode(type);
        camera.setParameters(params);

    }

    public int getTimer() {
        return Timer;
    }

    public void setTimer(int timer) {
        Timer = timer;
    }

    public String getFlash() {
        return params.getFlashMode();
    }

    public void setFocusPoint(Camera.Area focus, float x, float y, boolean lock) {
        params.setFocusAreas(Arrays.asList(focus));
        params.setAutoExposureLock(lock);
        params.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
        camera.setParameters(params);
        camera.autoFocus(focusListener);

        focusX = x;
        focusY = y;
        //this.invalidate();
        p.setColor(Color.WHITE);
        this.postInvalidate();
    }

    public void zoomIn() {
        int zoom = params.getZoom();
        if (zoom + 1 <= params.getMaxZoom()) {
            params.setZoom(zoom + 1);
        }
        camera.setParameters(params);
    }

    public void zooomOut() {
        int zoom = params.getZoom();
        if (zoom - 1 >= 0) {
            params.setZoom(zoom - 1);
        }
        camera.setParameters(params);
    }

    public void setOnPictureTaken(onPictureTakenListener onPictureTaken) {
        this.onPictureTaken = onPictureTaken;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (params.getFocusAreas() != null) {
            p.setStyle(Paint.Style.STROKE);
            p.setStrokeWidth(3);
            canvas.drawRect(focusX - 20, focusY - 20, focusX + 20, focusY + 20, p);
            /*canvas.drawLine(focusX, 0, focusX, this.getHeight(), p);
            canvas.drawLine(0, focusY, this.getWidth(), focusY, p);
            canvas.drawLine(focusX - 1, 0, focusX - 1, this.getHeight(), bp);
            canvas.drawLine(0, focusY - 1, this.getWidth(), focusY - 1, bp);
            canvas.drawLine(focusX + 1, 0, focusX + 1, this.getHeight(), bp);
            canvas.drawLine(0, focusY + 1, this.getWidth(), focusY + 1, bp);*/
        }
    }
}
