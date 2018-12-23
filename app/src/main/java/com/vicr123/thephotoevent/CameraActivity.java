/*
 *  Copyright 2017 The Android Open Source Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 *  thePhoto Event Mode Client: Sends photos to thePhoto on a PC
 *  Copyright (C) 2018 Victor Tran
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

package com.vicr123.thephotoevent;

import android.animation.Animator;
import android.annotation.SuppressLint;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.SensorManager;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.params.MeteringRectangle;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.media.MediaActionSound;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Build;
import android.os.CountDownTimer;
import android.os.Handler;
import android.os.HandlerThread;
import android.preference.PreferenceManager;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import android.util.Log;
import android.util.Size;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.SparseIntArray;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.ScaleGestureDetector;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.wearable.CapabilityClient;
import com.google.android.gms.wearable.CapabilityInfo;
import com.google.android.gms.wearable.MessageClient;
import com.google.android.gms.wearable.MessageEvent;
import com.google.android.gms.wearable.Node;
import com.google.android.gms.wearable.Wearable;
import com.google.android.material.bottomsheet.BottomSheetDialog;

import org.json.JSONObject;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import static com.vicr123.thephotoevent.Globals.sock;

class CompareSizesByArea implements Comparator<Size> {

    @Override
    public int compare(Size lhs, Size rhs) {
        // We cast here to ensure the multiplications won't overflow
        return Long.signum((long) lhs.getWidth() * lhs.getHeight() -
                (long) rhs.getWidth() * rhs.getHeight());
    }

}

public class CameraActivity extends AppCompatActivity {
    private static final int STATE_PREVIEW = 0;
    private static final int STATE_WAITING_LOCK = 1;
    private static final int STATE_WAITING_PRECAPTURE = 2;
    private static final int STATE_WAITING_NON_PRECAPTURE = 3;
    private static final int STATE_PICTURE_TAKEN = 4;
    private static final SparseIntArray ORIENTATIONS = new SparseIntArray();
    public static boolean currentlyRunning = false;
    int sensorOrientation;

    class TxViewGestures extends ScaleGestureDetector.SimpleOnScaleGestureListener implements View.OnTouchListener {
        float currentScaling = 1;
        ScaleGestureDetector scaleDetector;
        Rect originalSize;
        float maxZoom;
        boolean coordinateSystemPreCorrectionArraySize = false;

        public TxViewGestures(Context c) {
            scaleDetector = new ScaleGestureDetector(c, this);
        }

        public void newCamera() {
            try {
                CameraCharacteristics chars = ((CameraManager) getSystemService(CAMERA_SERVICE)).getCameraCharacteristics(cameraId);

                if (Build.VERSION.SDK_INT >= 23) {
                    originalSize = chars.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE);
                } else {
                    originalSize = chars.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
                }
                originalSize = new Rect(0, 0, originalSize.width(), originalSize.height());

                maxZoom = chars.get(CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM);

                currentScaling = 1;
                cameraHud.setScalingCircleScale(1);

                previewRequestBuilder.set(CaptureRequest.SCALER_CROP_REGION, originalSize);
            } catch (Exception e) {

            }
        }

        @Override
        public boolean onScale(ScaleGestureDetector scaleGestureDetector) {
            try {
                currentScaling *= scaleGestureDetector.getScaleFactor();
                if (currentScaling < 1) currentScaling = 1;
                if (currentScaling > 2) currentScaling = 2;
                if (currentScaling > maxZoom) currentScaling = maxZoom;

                cameraHud.setScalingCircleScale(currentScaling);

                if (currentScaling == 1) {
                    previewRequestBuilder.set(CaptureRequest.SCALER_CROP_REGION, originalSize);
                } else {
                    Rect scaledRect = new Rect(0, 0, (int) (originalSize.width() / currentScaling), (int) (originalSize.height() / currentScaling));
                    scaledRect.left = originalSize.width() / 2 - scaledRect.width() / 2;
                    scaledRect.top = originalSize.height() / 2 - scaledRect.height() / 2;
                    previewRequestBuilder.set(CaptureRequest.SCALER_CROP_REGION, scaledRect);
                }

                captureSession.stopRepeating();
                captureSession.setRepeatingRequest(previewRequestBuilder.build(), captureCallback, background);
            } catch (Exception e) {
                e.printStackTrace();
            }
            return true;
        }

        @Override
        public boolean onScaleBegin(ScaleGestureDetector scaleGestureDetector) {
            cameraHud.startScaling();
            return true;
        }

        @Override
        public void onScaleEnd(ScaleGestureDetector scaleGestureDetector) {
            cameraHud.stopScaling();
        }

        @Override
        public boolean onTouch(View view, MotionEvent motionEvent) {
            //Detect scaling gesture
            scaleDetector.onTouchEvent(motionEvent);

            switch (motionEvent.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    try {
                        CameraCharacteristics chars = ((CameraManager) getSystemService(CAMERA_SERVICE)).getCameraCharacteristics(cameraId);

                        Rect sensorArraySize = chars.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);

                        float x = (motionEvent.getY() / txView.getHeight()) * sensorArraySize.width() - 150;
                        float y = (motionEvent.getX() / txView.getWidth()) * sensorArraySize.height() - 150;

                        if (x < 0) x = 0;
                        if (y < 0) y = 0;

                        cameraHud.setAfPoint(new Point((int) motionEvent.getX(), (int) motionEvent.getY()));
                        cameraHud.setFixedFocus(true);

                        MeteringRectangle region = new MeteringRectangle((int) x, (int) y, 300, 300, MeteringRectangle.METERING_WEIGHT_MAX - 1);
                        MeteringRectangle[] regions = {
                                region
                        };

                        //Stop repeating request
                        captureSession.stopRepeating();

                        //Cancel any other AFs
                        previewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL);
                        previewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AE_MODE_OFF);
                        captureSession.capture(previewRequestBuilder.build(), null, background);


                        if (chars.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AF) >= 1) {
                            previewRequestBuilder.set(CaptureRequest.CONTROL_AF_REGIONS, regions);
                        }
                        if (chars.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AE) >= 1) {
                            previewRequestBuilder.set(CaptureRequest.CONTROL_AE_REGIONS, regions);
                        }

                        previewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START);
                        previewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_AUTO);
                        previewRequestBuilder.setTag("FOCUS");
                        lockAF = true;
                        processingAF = true;

                        //Fire one request
                        captureSession.capture(previewRequestBuilder.build(), new CameraCaptureSession.CaptureCallback() {
                            @Override
                            public void onCaptureCompleted(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull TotalCaptureResult result) {
                                super.onCaptureCompleted(session, request, result);

                                previewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_IDLE);
                                previewRequestBuilder.setTag(null);
                                processingAF = false;

                                //Start preview again
                                try {
                                    captureSession.setRepeatingRequest(previewRequestBuilder.build(), captureCallback, background);
                                } catch (Exception e) {
                                    e.printStackTrace();
                                }
                            }
                        }, background);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    break;
            }
            return true;
        }
    }

    OrientationEventListener orientationListener;

    static {
        ORIENTATIONS.append(Surface.ROTATION_0, 90);
        ORIENTATIONS.append(Surface.ROTATION_90, 0);
        ORIENTATIONS.append(Surface.ROTATION_180, 270);
        ORIENTATIONS.append(Surface.ROTATION_270, 180);
    }

    LinearLayout imagesLayout;
    AutoFitTextureView txView;
    ImageReader imageReader;
    Handler background;
    HandlerThread backgroundThread;
    Size previewSize;
    String cameraId;
    CameraDevice device;
    CaptureRequest.Builder previewRequestBuilder;
    CameraCaptureSession captureSession;
    Semaphore cameraOpenCloseLock = new Semaphore(1);
    MediaActionSound shutterSound;
    CameraHudView cameraHud;
    TxViewGestures txGestures;
    MessageClient wearableMessageClient;
    MessageClient.OnMessageReceivedListener wearableMessageReceivedListener;
    int state = STATE_PREVIEW;
    int currentOrientation = 0, currentCardinalOrientation = 0;
    int flashType = CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH;
    int currentCameraOrientation = CameraCharacteristics.LENS_FACING_BACK;
    boolean lockAF = false;
    boolean processingAF = false;
    AlertDialog reconnectingDialog;

    boolean canFlash, canFlip;

    CountDownTimer captureTimer;
    int timerDelay;
    MediaPlayer timerHighBeep, timerBeep, timer10, timer3;

    BroadcastReceiver closeReceiver;
    boolean userRequestedCloseFromNotification = false;

    CameraCaptureSession.CaptureCallback captureCallback = new CameraCaptureSession.CaptureCallback() {
        private void process(CaptureResult result) {
            final Integer afState = result.get(CaptureResult.CONTROL_AF_STATE);
            if (afState != null) {
                //Tell the HUD about the state
                //Needs to be put on the end of the event loop for some reason
                cameraHud.post(new Runnable() {
                    @Override
                    public void run() {
                        cameraHud.setAfState(afState);
                    }
                });
            }

            switch (state) {
                case STATE_PREVIEW: {
                    // We have nothing to do when the camera preview is working normally.
                    break;
                }
                case STATE_WAITING_LOCK: {
                    if (afState == null) {
                        //Wait a bit to see if the state comes in
                        background.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                if (state == STATE_WAITING_LOCK) {
                                    state = STATE_PICTURE_TAKEN;
                                    captureStillPicture();
                                }
                            }
                        }, 250);
                    } else if (CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED == afState || CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED == afState) {
                        Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                        if (aeState == null || aeState == CaptureResult.CONTROL_AE_STATE_CONVERGED) {
                            state = STATE_PICTURE_TAKEN;
                            captureStillPicture();
                        } else {
                            try {
                                previewRequestBuilder.set(CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER, CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER_START);
                                state = STATE_WAITING_PRECAPTURE;
                                captureSession.capture(previewRequestBuilder.build(), captureCallback, background);
                            } catch (CameraAccessException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                    break;
                }
                case STATE_WAITING_PRECAPTURE: {
                    // CONTROL_AE_STATE can be null on some devices
                    Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                    if (aeState == null || aeState == CaptureResult.CONTROL_AE_STATE_PRECAPTURE || aeState == CaptureRequest.CONTROL_AE_STATE_FLASH_REQUIRED) {
                        state = STATE_WAITING_NON_PRECAPTURE;
                    }
                    break;
                }
                case STATE_WAITING_NON_PRECAPTURE: {
                    // CONTROL_AE_STATE can be null on some devices
                    Integer aeState = result.get(CaptureResult.CONTROL_AE_STATE);
                    if (aeState == null || aeState != CaptureResult.CONTROL_AE_STATE_PRECAPTURE) {
                        state = STATE_PICTURE_TAKEN;
                        captureStillPicture();
                    }
                    break;
                }
            }
        }

        @Override
        public void onCaptureProgressed(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull CaptureResult partialResult) {
            process(partialResult);
        }

        @Override
        public void onCaptureCompleted(@NonNull CameraCaptureSession session, @NonNull CaptureRequest request, @NonNull TotalCaptureResult result) {
            process(result);
        }
    };

    private void captureStillPicture() {
        try {
            if (device == null) {
                return;
            }
            // This is the CaptureRequest.Builder that we use to take a picture.
            final CaptureRequest.Builder captureBuilder = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            captureBuilder.addTarget(imageReader.getSurface());

            captureBuilder.set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO);

            // Use the same AE and AF modes as the preview.
            captureBuilder.set(CaptureRequest.CONTROL_AF_MODE, previewRequestBuilder.get(CaptureRequest.CONTROL_AF_MODE));
            captureBuilder.set(CaptureRequest.CONTROL_AE_MODE, previewRequestBuilder.get(CaptureRequest.CONTROL_AE_MODE));
            captureBuilder.set(CaptureRequest.CONTROL_AF_REGIONS, previewRequestBuilder.get(CaptureRequest.CONTROL_AF_REGIONS));
            captureBuilder.set(CaptureRequest.CONTROL_AE_REGIONS, previewRequestBuilder.get(CaptureRequest.CONTROL_AE_REGIONS));

            // Orientation
            captureBuilder.set(CaptureRequest.JPEG_ORIENTATION, getOrientation(currentOrientation));

            CameraCaptureSession.CaptureCallback CaptureCallback = new CameraCaptureSession.CaptureCallback() {
                @Override
                public void onCaptureCompleted(@NonNull CameraCaptureSession session,
                                               @NonNull CaptureRequest request,
                                               @NonNull TotalCaptureResult result) {
                    unlockFocus();
                }
            };

            captureSession.stopRepeating();
            captureSession.abortCaptures();
            captureSession.capture(captureBuilder.build(), CaptureCallback, null);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void lockFocus() {
        try {
            // This is how to tell the camera to lock focus.
            previewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START);
            // Tell #mCaptureCallback to wait for the lock.
            state = STATE_WAITING_LOCK;
            captureSession.capture(previewRequestBuilder.build(), captureCallback, background);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void unlockFocus() {
        try {
            // Reset the auto-focus trigger
            previewRequestBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL);
            previewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, flashType);
            captureSession.capture(previewRequestBuilder.build(), captureCallback, background);
            // After this, the camera will go back to the normal state of preview.
            state = STATE_PREVIEW;
            captureSession.setRepeatingRequest(previewRequestBuilder.build(), captureCallback, background);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        } catch (IllegalStateException e) {
            //Recreate the camera session
            //createCameraPreviewSession();
            closeCamera();
            openCamera(txView.getWidth(), txView.getHeight());
        }
    }

    private int getOrientation(int rotation) {
        // Sensor orientation is 90 for most devices, or 270 for some devices (eg. Nexus 5X)
        // We have to take that into account and rotate JPEG properly.
        // For devices with orientation of 90, we simply return our mapping from ORIENTATIONS.
        // For devices with orientation of 270, we need to rotate the JPEG 180 degrees.

        int orientation;
        if (rotation < 45) {
            orientation = 90;
        } else if (rotation < 135) {
            orientation = 0;
        } else if (rotation < 225) {
            orientation = 270;
        } else if (rotation < 315) {
            orientation = 180;
        } else {
            orientation = 90;
        }

        /*int orientation;
        if (rotation < 45) {
            orientation = 0;
        } else if (rotation < 135) {
            orientation = 90;
        } else if (rotation < 225) {
            orientation = 180;
        } else if (rotation < 315) {
            orientation = 270;
        } else {
            orientation = 0;
        }*/

        return (orientation + sensorOrientation + 270) % 360;
        //return (sensorOrientation + orientation + 360) % 360;
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);

        if (sock == null) {
            Intent i = new Intent(this, MainActivity.class);
            startActivity(i);
            finish();
            return;
        }

        txView = findViewById(R.id.viewfinder);
        imagesLayout = findViewById(R.id.pendingImagesContainer);
        cameraHud = findViewById(R.id.cameraHudView);

        final CameraActivity context = this;
        sock.setOnClose(new Runnable() {
            @Override
            public void run() {
                if (sock.isReconnecting()) {
                    reconnectingDialog.dismiss();

                    AlertDialog.Builder builder = new AlertDialog.Builder(context);
                    builder.setTitle(R.string.disconnected_notification_title);
                    builder.setMessage(R.string.disconnected_dialog_text);
                    builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            dialogInterface.dismiss();
                            finish();
                        }
                    });
                    builder.show();
                } else if (!context.hasWindowFocus() && !userRequestedCloseFromNotification) {
                    Intent intent = new Intent(context, MainActivity.class);
                    PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, intent, 0);

                    NotificationCompat.Builder builder = new NotificationCompat.Builder(context, "connected_notification")
                            .setSmallIcon(R.drawable.ic_notification_icon)
                            .setContentTitle(getString(R.string.disconnected_notification_title))
                            .setContentText(getString(R.string.disconnected_notification_text))
                            .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                            .setStyle(new NotificationCompat.BigTextStyle().bigText(getString(R.string.disconnected_notification_text)))
                            .setContentIntent(pendingIntent)
                            .setColor(getResources().getColor(R.color.colorPrimary));

                    NotificationManagerCompat mgr = NotificationManagerCompat.from(context);
                    mgr.notify(1, builder.build());
                    finish();
                }
            }
        });
        sock.setOnUnexpectedDisconnect(new Runnable() {
            @Override
            public void run() {
                AlertDialog.Builder builder = new AlertDialog.Builder(context);
                builder.setCancelable(false);
                builder.setView(R.layout.dialog_reconnect_needed);

                reconnectingDialog = builder.create();
                sock.setOnReconnect(new Runnable() {
                    @Override
                    public void run() {
                        reconnectingDialog.dismiss();
                    }
                });
                reconnectingDialog.show();
            }
        });

        txGestures = new TxViewGestures(this);
        txView.setOnTouchListener(txGestures);

        orientationListener = new OrientationEventListener(this, SensorManager.SENSOR_DELAY_NORMAL) {
            @Override
            public void onOrientationChanged(int o) {
                if (o != OrientationEventListener.ORIENTATION_UNKNOWN) {
                    currentOrientation = 360 - o;

                    int newCardinal;
                    if (currentOrientation < 45) {
                        newCardinal = 0;
                    } else if (currentOrientation < 135) {
                        newCardinal = 90;
                    } else if (currentOrientation < 225) {
                        newCardinal = 180;
                    } else if (currentOrientation < 315) {
                        newCardinal = 270;
                    } else {
                        newCardinal = 0;
                    }

                    if (currentCardinalOrientation != newCardinal) {
                        currentCardinalOrientation = newCardinal;

                        rotateViews();
                    }
                }
            }
        };

        timerHighBeep = MediaPlayer.create(this, R.raw.timer_beep_sound_high);
        timerBeep = MediaPlayer.create(this, R.raw.timer_beep_sound);
        timer10 = MediaPlayer.create(this, R.raw.tenseconds);
        timer3 = MediaPlayer.create(this, R.raw.threeseconds);
        shutterSound = new MediaActionSound();

        //Set up connected notification
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            //Set up notification channel on Android Oreo
            NotificationChannel channel = new NotificationChannel("connected_notification", getString(R.string.connected_notification_channel_name), NotificationManager.IMPORTANCE_LOW);
            channel.setDescription(getString(R.string.connected_notification_channel_description));

            NotificationManager mgr = getSystemService(NotificationManager.class);
            mgr.createNotificationChannel(channel);
        }

        Intent openIntent = new Intent(this, CameraActivity.class);
        openIntent.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
        PendingIntent pendingOpenIntent = PendingIntent.getActivity(this, 0, openIntent, 0);

        IntentFilter disconnectIntentFilter = new IntentFilter("android.intent.CLOSE_ACTIVITY");
        closeReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                userRequestedCloseFromNotification = true;
                finish();
            }
        };
        registerReceiver(closeReceiver, disconnectIntentFilter);

        Intent disconnectIntent = new Intent("android.intent.CLOSE_ACTIVITY");
        PendingIntent pendingDisconnectIntent = PendingIntent.getBroadcast(this, 0, disconnectIntent, 0);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, "connected_notification")
                .setSmallIcon(R.drawable.ic_notification_icon)
                .setContentTitle(getString(R.string.connected_notification_title))
                .setContentText(getString(R.string.connected_notification_text))
                .setOngoing(true)
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setStyle(new NotificationCompat.BigTextStyle().bigText(getString(R.string.connected_notification_text)))
                .setContentIntent(pendingOpenIntent)
                .addAction(R.drawable.ic_close_black_24dp, getString(R.string.connected_notification_disconnect_button), pendingDisconnectIntent)
                .setColor(getResources().getColor(R.color.colorPrimary));

        NotificationManagerCompat mgr = NotificationManagerCompat.from(this);
        mgr.notify(0, builder.build());

        currentlyRunning = true;

        //Tell Wear OS that we're in
        wearableMessageClient = Wearable.getMessageClient(this);
        wearableMessageReceivedListener = new MessageClient.OnMessageReceivedListener() {
            @Override
            public void onMessageReceived(@NonNull MessageEvent messageEvent) {
                if (messageEvent.getPath().equals("/cameraaction")) {
                    try {
                        String data = new String(messageEvent.getData(), "UTF-8");

                        if (data.equals("CAPTURE")) {
                            Capture(null);
                        }
                    } catch (UnsupportedEncodingException e) {
                        Log.wtf("wearable_message", e);
                    }
                } else if (messageEvent.getPath().equals("/cameraaction/stateupdate")) {
                    try {
                        JSONObject state = new JSONObject(new String(messageEvent.getData(), "UTF-8"));

                        setFlash(state.getInt("flash"));
                        setTimer(state.getInt("timer"));
                    } catch (Exception e) {
                        Log.wtf("wearable_message", e);
                    }
                } else if (messageEvent.getPath().equals("/cameraaction/requeststate")) {
                    sendStateUpdateMessage();
                }
            }
        };
    }

    @Override
    protected void onResume() {
        super.onResume();
        startBackgroundThread();
        orientationListener.enable();

        if (txView.isAvailable()) {
            openCamera(txView.getWidth(), txView.getHeight());
        } else {
            txView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
                @Override
                public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int width, int height) {
                    openCamera(width, height);
                }

                @Override
                public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int width, int height) {
                    configureTransform(width, height);
                }

                @Override
                public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
                    return false;
                }

                @Override
                public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {

                }
            });
        }

        //Tell Wear OS that we exist
        Wearable.getCapabilityClient(this).addLocalCapability("camera_active");
        Wearable.getCapabilityClient(this).getCapability("check_capabilities", CapabilityClient.FILTER_REACHABLE)
                .addOnCompleteListener(new OnCompleteListener<CapabilityInfo>() {
            @Override
            public void onComplete(@NonNull Task<CapabilityInfo> task) {
                if (task.isSuccessful()) {
                    for (Node n : task.getResult().getNodes()) {
                        if (n.isNearby()) {
                            Task<Integer> launchTask = wearableMessageClient.sendMessage(n.getId(), "/capcheck", null);
                        }
                    }
                }
            }
        });
        wearableMessageClient.addListener(wearableMessageReceivedListener, Uri.parse("wear://*/cameraaction"), MessageClient.FILTER_PREFIX);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus) {
            View decorView = getWindow().getDecorView();
            decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN);
        }
    }

    @Override
    protected void onPause() {
        closeCamera();
        stopBackgroundThread();
        orientationListener.disable();

        //Tell Wear OS that we don't exist
        Wearable.getCapabilityClient(this).removeLocalCapability("camera_active");
        Wearable.getCapabilityClient(this).getCapability("check_capabilities", CapabilityClient.FILTER_REACHABLE)
                .addOnCompleteListener(new OnCompleteListener<CapabilityInfo>() {
                    @Override
                    public void onComplete(@NonNull Task<CapabilityInfo> task) {
                        if (task.isSuccessful()) {
                            for (Node n : task.getResult().getNodes()) {
                                if (n.isNearby()) {
                                    Task<Integer> launchTask = wearableMessageClient.sendMessage(n.getId(), "/capcheck", null);
                                }
                            }
                        }
                    }
                });
        wearableMessageClient.removeListener(wearableMessageReceivedListener);

        super.onPause();
    }

    private void openCamera(int width, int height) {
        openCamera(width, height, currentCameraOrientation);
    }

    private void openCamera(int width, int height, int preferredCameraOrientation) {
        //Set up camera variables
        setUpCameraOutputs(width, height, preferredCameraOrientation);
        configureTransform(width, height);

        CameraManager mgr = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            if (!cameraOpenCloseLock.tryAcquire(3000, TimeUnit.MILLISECONDS)) {
                throw new RuntimeException("Cannot acquire lock");
            }
            mgr.openCamera(cameraId, new CameraDevice.StateCallback() {
                @Override
                public void onOpened(@NonNull CameraDevice cameraDevice) {
                    cameraOpenCloseLock.release();
                    device = cameraDevice;
                    createCameraPreviewSession();
                }

                @Override
                public void onDisconnected(@NonNull CameraDevice cameraDevice) {
                    cameraOpenCloseLock.release();
                    device.close();
                    device = null;
                }

                @Override
                public void onError(@NonNull CameraDevice cameraDevice, int i) {
                    cameraOpenCloseLock.release();
                    device.close();
                    device = null;
                }
            }, background);
        } catch (SecurityException e) {

        } catch (Exception e) {

        }
    }

    private void setUpCameraOutputs(int width, int height, int preferredCameraOrientation) {
        CameraManager mgr = (CameraManager) getSystemService(Context.CAMERA_SERVICE);
        try {
            ArrayList<String> preferredCameras = new ArrayList<>();
            ArrayList<String> allAvailableCameras = new ArrayList<>();

            for (String cameraId : mgr.getCameraIdList()) {
                CameraCharacteristics chars = mgr.getCameraCharacteristics(cameraId);
                StreamConfigurationMap map = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
                if (map != null) {
                    allAvailableCameras.add(cameraId);

                    //Check if the preferred orientation is correct
                    Integer cameraFacing = chars.get(CameraCharacteristics.LENS_FACING);
                    if (cameraFacing != null && cameraFacing == preferredCameraOrientation) {
                        preferredCameras.add(cameraId);
                    }
                }
            }

            if (preferredCameras.size() != 0) {
                cameraId = preferredCameras.get(0);
            } else {
                cameraId = allAvailableCameras.get(0);
            }

            txGestures.newCamera();

            if (allAvailableCameras.size() == 0) {
                findViewById(R.id.button_flip).setVisibility(View.GONE);
                canFlip = false;
            } else {
                findViewById(R.id.button_flip).setVisibility(View.VISIBLE);
                canFlip = true;
            }

            CameraCharacteristics chars = mgr.getCameraCharacteristics(cameraId);

            currentCameraOrientation = chars.get(CameraCharacteristics.LENS_FACING);

            StreamConfigurationMap map = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            Size largest = Collections.max(Arrays.asList(map.getOutputSizes(ImageFormat.JPEG)), new CompareSizesByArea());
            refreshImageReader(largest);

            // Find out if we need to swap dimension to get the preview size relative to sensor
            // coordinate.
            int displayRotation = getWindowManager().getDefaultDisplay().getRotation();
            //noinspection ConstantConditions
            sensorOrientation = chars.get(CameraCharacteristics.SENSOR_ORIENTATION);

            boolean swappedDimensions = false;
            switch (displayRotation) {
                case Surface.ROTATION_0:
                case Surface.ROTATION_180:
                    if (sensorOrientation == 90 || sensorOrientation == 270) {
                        swappedDimensions = true;
                    }
                    break;
                case Surface.ROTATION_90:
                case Surface.ROTATION_270:
                    if (sensorOrientation == 0 || sensorOrientation == 180) {
                        swappedDimensions = true;
                    }
                    break;
            }

            Point displaySize = new Point();
            getWindowManager().getDefaultDisplay().getSize(displaySize);
            int rotatedPreviewWidth = width;
            int rotatedPreviewHeight = height;
            int maxPreviewWidth = displaySize.x;
            int maxPreviewHeight = displaySize.y;

            if (swappedDimensions) {
                rotatedPreviewWidth = height;
                rotatedPreviewHeight = width;
                maxPreviewWidth = displaySize.y;
                maxPreviewHeight = displaySize.x;
            }

            if (maxPreviewWidth > 1920) maxPreviewWidth = 1920;
            if (maxPreviewHeight > 1080) maxPreviewWidth = 1080;

            Boolean flashAvailable = chars.get(CameraCharacteristics.FLASH_INFO_AVAILABLE);
            if (flashAvailable == null || !flashAvailable) {
                findViewById(R.id.button_flash).setVisibility(View.GONE);
                canFlash = false;
            } else {
                findViewById(R.id.button_flash).setVisibility(View.VISIBLE);
                canFlash = true;
            }

            //Choose a size for the preview
            previewSize = chooseOptimalSize(map.getOutputSizes(SurfaceTexture.class), rotatedPreviewWidth, rotatedPreviewHeight, maxPreviewWidth, maxPreviewHeight, largest);
            txView.setAspectRatio(previewSize.getHeight(), previewSize.getWidth());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void refreshImageReader(final Size largest) {
        final CameraActivity activity = this;
        imageReader = ImageReader.newInstance(largest.getWidth(), largest.getHeight(), ImageFormat.JPEG, 5);
        imageReader.setOnImageAvailableListener(new ImageReader.OnImageAvailableListener() {
            @Override
            public void onImageAvailable(ImageReader imageReader) {
                if (processingAF) {
                    imageReader.acquireNextImage().close();
                    //refreshImageReader(largest);
                    return; //Don't do anything
                }
                shutterSound.play(MediaActionSound.SHUTTER_CLICK);

                Image image = imageReader.acquireLatestImage();
                ByteBuffer buffer = image.getPlanes()[0].getBuffer();
                byte[] bytes = new byte[buffer.capacity()];
                buffer.get(bytes);
                final Bitmap bitmapImage = BitmapFactory.decodeByteArray(bytes, 0, bytes.length, null);
                image.close();

                //refreshImageReader(largest);

                final ProgressBar progress = new ProgressBar(activity, null, android.R.attr.progressBarStyleHorizontal);
                //spinner.setVisibility(View.GONE);

                final FrameLayout layout = new FrameLayout(activity);

                imagesLayout.getHandler().post(new Runnable() {
                    @Override
                    public void run() {
                        LinearLayout.LayoutParams imagesLayoutParams = new LinearLayout.LayoutParams(imagesLayout.getHeight(), imagesLayout.getHeight());
                        imagesLayoutParams.setMargins(0, 0, 0, 0);
                        imagesLayoutParams.gravity = Gravity.START;

                        FrameLayout.LayoutParams frameLayoutParams = new FrameLayout.LayoutParams(imagesLayout.getHeight(), imagesLayout.getHeight());
                        layout.setLayoutParams(frameLayoutParams);

                        final ImageView view = new ImageView(activity);
                        view.setImageBitmap(bitmapImage);
                        view.setLayoutParams(imagesLayoutParams);
                        layout.addView(view);

                        RelativeLayout progLayout = new RelativeLayout(activity);
                        progLayout.addView(progress);

                        RelativeLayout.LayoutParams progParams = new RelativeLayout.LayoutParams(imagesLayout.getHeight(), imagesLayout.getHeight());
                        progParams.addRule(RelativeLayout.ALIGN_PARENT_TOP, progress.getId());
                        progParams.addRule(RelativeLayout.ALIGN_PARENT_END, progress.getId());
                        progParams.addRule(RelativeLayout.ALIGN_PARENT_START, progress.getId());
                        progLayout.setLayoutParams(progParams);
                        layout.addView(progLayout);

                        imagesLayout.addView(layout);

                        imagesLayout.measure(View.MeasureSpec.makeMeasureSpec(imagesLayout.getMeasuredWidth(), View.MeasureSpec.EXACTLY), View.MeasureSpec.makeMeasureSpec(imagesLayout.getMeasuredHeight(), View.MeasureSpec.EXACTLY));
                        imagesLayout.layout(imagesLayout.getLeft(), imagesLayout.getTop(), imagesLayout.getRight(), imagesLayout.getBottom());
                    }
                });

                //refreshImageReader(largest);
                createCameraPreviewSession();
                openCamera(txView.getWidth(), txView.getHeight());

                sock.writeImage(bytes, new SendImageCallback() {
                    @Override
                    public void beforeSend() {
                        progress.setVisibility(View.VISIBLE);
                    }

                    @Override
                    public void afterSend() {
                        imagesLayout.getHandler().post(new Runnable() {
                               @Override
                               public void run() {
                                   imagesLayout.removeView(layout);
                               }
                           });
                    }

                    @Override
                    public void progress(int bytesSent, int bytesTotal) {
                        progress.setMax(bytesTotal);
                        progress.setProgress(bytesSent);
                    }
                });
            }
        }, background);
    }

    private void configureTransform(int width, int height) {
        if (txView == null || previewSize == null) {
            return;
        }

        int rotation = getWindowManager().getDefaultDisplay().getRotation();
        final Matrix matrix = new Matrix();
        RectF viewRect = new RectF(0, 0, width, height);
        RectF bufferRect = new RectF(0, 0, previewSize.getHeight(), previewSize.getWidth());
        float centerX = viewRect.centerX();
        float centerY = viewRect.centerY();
        if (Surface.ROTATION_90 == rotation || Surface.ROTATION_270 == rotation) {
            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY());
            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL);
            float scale = Math.max(
                    (float) height / previewSize.getHeight(),
                    (float) width / previewSize.getWidth());
            matrix.postScale(scale, scale, centerX, centerY);
            matrix.postRotate(90 * (rotation - 2), centerX, centerY);
        } else if (Surface.ROTATION_180 == rotation) {
            matrix.postRotate(180, centerX, centerY);
        }

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                txView.setTransform(matrix);
            }
        });
    }

    private void createCameraPreviewSession() {
        try {
            SurfaceTexture texture = txView.getSurfaceTexture();
            assert texture != null;

            // We configure the size of default buffer to be the size of camera preview we want.
            texture.setDefaultBufferSize(previewSize.getWidth(), previewSize.getHeight());

            // This is the output Surface we need to start preview.
            Surface surface = new Surface(texture);

            // We set up a CaptureRequest.Builder with the output Surface.
            previewRequestBuilder = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            previewRequestBuilder.addTarget(surface);

            // Here, we create a CameraCaptureSession for camera preview.
            device.createCaptureSession(Arrays.asList(surface, imageReader.getSurface()), new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                            // The camera is already closed
                            if (null == device) {
                                return;
                            }

                            // When the session is ready, we start displaying the preview.
                            captureSession = cameraCaptureSession;
                            try {
                                if (lockAF) {
                                    //Set AF to auto
                                    previewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_AUTO);
                                } else {
                                    //Set AF to continuous
                                    previewRequestBuilder.set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE);
                                }
                                previewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, flashType);

                                // Finally, we start displaying the camera preview.
                                captureSession.setRepeatingRequest(previewRequestBuilder.build(), captureCallback, background);
                            } catch (CameraAccessException e) {
                                e.printStackTrace();
                            }
                        }

                        @Override
                        public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                            //showToast("Failed");
                        }
                    }, null
            );
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    private void closeCamera() {
        try {
            cameraOpenCloseLock.acquire();
            if (captureSession != null) {
                captureSession.close();
                captureSession = null;
            }

            if (device != null) {
                device.close();
                device = null;
            }

            if (imageReader != null) {
                imageReader.close();
                imageReader = null;
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            cameraOpenCloseLock.release();
        }
    }

    private static Size chooseOptimalSize(Size[] choices, int textureViewWidth, int textureViewHeight, int maxWidth, int maxHeight, Size aspectRatio) {
        // Collect the supported resolutions that are at least as big as the preview Surface
        List<Size> bigEnough = new ArrayList<>();
        // Collect the supported resolutions that are smaller than the preview Surface
        List<Size> notBigEnough = new ArrayList<>();
        int w = aspectRatio.getWidth();
        int h = aspectRatio.getHeight();
        for (Size option : choices) {
            if (option.getWidth() <= maxWidth && option.getHeight() <= maxHeight &&
                    option.getHeight() == option.getWidth() * h / w) {
                if (option.getWidth() >= textureViewWidth &&
                        option.getHeight() >= textureViewHeight) {
                    bigEnough.add(option);
                } else {
                    notBigEnough.add(option);
                }
            }
        }

        // Pick the smallest of those big enough. If there is no one big enough, pick the
        // largest of those not big enough.
        if (bigEnough.size() > 0) {
            return Collections.min(bigEnough, new CompareSizesByArea());
        } else if (notBigEnough.size() > 0) {
            return Collections.max(notBigEnough, new CompareSizesByArea());
        } else {
            //Log.e(TAG, "Couldn't find any suitable preview size");
            return choices[0];
        }
    }

    private void startBackgroundThread() {
        backgroundThread = new HandlerThread("CameraAsync");
        backgroundThread.start();
        background = new Handler(backgroundThread.getLooper());
    }

    private void stopBackgroundThread() {
        backgroundThread.quitSafely();
        try {
            backgroundThread.join();
            backgroundThread = null;
            background = null;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @SuppressLint("WrongViewCast")
    void hideAuxiliaryButtons() {
        View[] viewsToChange = {
                canFlash ? findViewById(R.id.button_flash) : null,
                findViewById(R.id.button_timer),
                canFlip ? findViewById(R.id.button_flip) : null
        };

        for (final View v : viewsToChange) {
            if (v != null) {
                v.animate()
                        .alpha(0f)
                        .setDuration(500)
                        .setListener(new Animator.AnimatorListener() {
                            @Override
                            public void onAnimationStart(Animator animator) {

                            }

                            @Override
                            public void onAnimationEnd(Animator animator) {
                                v.setVisibility(View.GONE);
                            }

                            @Override
                            public void onAnimationCancel(Animator animator) {

                            }

                            @Override
                            public void onAnimationRepeat(Animator animator) {

                            }
                        });
            }
        }
    }

    @SuppressLint("WrongViewCast")
    void showAuxiliaryButtons() {
        View[] viewsToChange = {
                canFlash ? findViewById(R.id.button_flash) : null,
                findViewById(R.id.button_timer),
                canFlip ? findViewById(R.id.button_flip) : null
        };

        for (View v : viewsToChange) {
            if (v != null) {
                v.setVisibility(View.VISIBLE);
                v.animate()
                        .alpha(1f)
                        .setDuration(500)
                        .setListener(null);
            }
        }
    }

    void rotateViews() {
        View[] viewsToChange = {
                findViewById(R.id.button_flash),
                findViewById(R.id.button_timer),
                findViewById(R.id.button_flip),
                findViewById(R.id.timer_hud_text)
        };

        for (View v : viewsToChange) {
            if (v != null) {
                v.animate()
                        .rotation(currentCardinalOrientation)
                        .setDuration(500)
                        .setListener(null);
            }
        }
    }

    public void Capture(View view) {
        final TextView hudText = findViewById(R.id.timer_hud_text);

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        final String soundType = prefs.getString("sound_type", "ping");

        if (captureTimer == null) {
            hudText.setVisibility(View.VISIBLE);
            hudText.setText(Integer.toString(timerDelay / 1000));

            if (soundType.equals("vrabbers")) {
                if (timerDelay == 10000) {
                    timer10.start();
                } else if (timerDelay == 3000) {
                    timer3.start();
                }
            }

            //Start counting down
            final CameraActivity activity = this;
            hideAuxiliaryButtons();
            captureTimer = new CountDownTimer(timerDelay, 1000) {
                @Override
                public void onTick(long l) {
                    int seconds = (int) Math.ceil(l / 1000) + 1;
                    sock.sendCommand("TIMER " + seconds);
                    hudText.setText(Integer.toString(seconds));

                    if (!soundType.equals("vrabbers")) {
                        if (seconds <= 3) {
                            timerHighBeep.start();

                            //Vibrate all Wear OS devices
                            Wearable.getCapabilityClient(activity).getCapability("camera_message", CapabilityClient.FILTER_REACHABLE)
                                    .addOnCompleteListener(new OnCompleteListener<CapabilityInfo>() {
                                        @Override
                                        public void onComplete(@NonNull Task<CapabilityInfo> task) {
                                            if (task.isSuccessful()) {
                                                for (Node n : task.getResult().getNodes()) {
                                                    if (n.isNearby()) {
                                                        try {
                                                            Task<Integer> vibrateTask = wearableMessageClient.sendMessage(n.getId(), "/cameraaction/vibrate", ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(100).array());
                                                        } catch (Exception e) {
                                                            Log.wtf("TIMER_WEAR_VIBRATE", e);
                                                        }

                                                    }
                                                }
                                            }
                                        }
                                    });
                        } else {
                            timerBeep.start();
                        }
                    }
                }

                @Override
                public void onFinish() {
                    sock.sendCommand("TIMER 0");
                    lockFocus();
                    captureTimer = null;
                    showAuxiliaryButtons();
                    hudText.setVisibility(View.GONE);
                }
            };
            captureTimer.start();
        } else {
            //Cancel picture
            sock.sendCommand("TIMER 0");
            captureTimer.cancel();
            captureTimer = null;
            showAuxiliaryButtons();
            hudText.setVisibility(View.GONE);

            if (soundType.equals("vrabbers")) {
                if (timer10.isPlaying()) timer10.stop();
                if (timer3.isPlaying()) timer3.stop();
            }
        }
    }

    void setFlash(int flashSetting) {
        switch (flashSetting) {
            case 0: //Auto
                flashType = CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH;
                findViewById(R.id.button_flash).setBackgroundResource(R.drawable.button_flashauto);
                previewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, flashType);
                previewRequestBuilder.set(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF);
                break;
            case 1: //Off
                flashType = CaptureRequest.CONTROL_AE_MODE_ON;
                findViewById(R.id.button_flash).setBackgroundResource(R.drawable.button_flashoff);
                previewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, flashType);
                previewRequestBuilder.set(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_OFF);
                break;
            case 2: //On
                flashType = CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH;
                findViewById(R.id.button_flash).setBackgroundResource(R.drawable.button_flashon);
                previewRequestBuilder.set(CaptureRequest.CONTROL_AE_MODE, flashType);
                previewRequestBuilder.set(CaptureRequest.FLASH_MODE, CaptureRequest.FLASH_MODE_SINGLE);
                break;
        }

        //Refresh preview
        try {
            captureSession.setRepeatingRequest(previewRequestBuilder.build(), captureCallback, background);
        } catch (CameraAccessException e) {
            e.printStackTrace();
        }
    }

    public void changeFlash(View view) {
        final BottomSheetDialog dialog = new BottomSheetDialog(this);

        View content = getLayoutInflater().inflate(R.layout.dialog_select_flash, null);

        content.findViewById(R.id.flash_dialog_automatic).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setFlash(0);
                sendStateUpdateMessage();
                dialog.dismiss();
            }
        });
        content.findViewById(R.id.flash_dialog_off).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setFlash(1);
                sendStateUpdateMessage();
                dialog.dismiss();
            }
        });
        content.findViewById(R.id.flash_dialog_on).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setFlash(2);
                sendStateUpdateMessage();
                dialog.dismiss();
            }
        });

        dialog.setContentView(content);
        dialog.show();
    }

    void setTimer(int timerSetting) {
        switch (timerSetting) {
            case 0: //0 sec
                findViewById(R.id.button_timer).setBackgroundResource(R.drawable.button_timer0);
                timerDelay = 0;
                sendStateUpdateMessage();
                break;
            case 1: //3 sec
                findViewById(R.id.button_timer).setBackgroundResource(R.drawable.button_timer3);
                timerDelay = 3000;
                sendStateUpdateMessage();
                break;
            case 2: //10 sec
                findViewById(R.id.button_timer).setBackgroundResource(R.drawable.button_timer10);
                timerDelay = 10000;
                sendStateUpdateMessage();
                break;
        }
    }

    public void changeTimer(View view) {
        final BottomSheetDialog dialog = new BottomSheetDialog(this);

        View content = getLayoutInflater().inflate(R.layout.dialog_select_timer, null);

        content.findViewById(R.id.timer_dialog_0sec).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setTimer(0);
                dialog.dismiss();
            }
        });
        content.findViewById(R.id.timer_dialog_3sec).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setTimer(1);
                dialog.dismiss();
            }
        });
        content.findViewById(R.id.timer_dialog_10sec).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                setTimer(2);
                dialog.dismiss();
            }
        });

        dialog.setContentView(content);
        dialog.show();
    }

    public void flipCamera(View view) {
        closeCamera();

        if (currentCameraOrientation == CameraCharacteristics.LENS_FACING_BACK) {
            openCamera(txView.getWidth(), txView.getHeight(), CameraCharacteristics.LENS_FACING_FRONT);
        } else {
            openCamera(txView.getWidth(), txView.getHeight(), CameraCharacteristics.LENS_FACING_BACK);
        }
    }


    public void sendStateUpdateMessage() {
        try {
            final JSONObject stateUpdate = new JSONObject();

            switch (flashType) {
                case CaptureRequest.CONTROL_AE_MODE_ON_AUTO_FLASH:
                    stateUpdate.put("flash", 0);
                    break;
                case CaptureRequest.CONTROL_AE_MODE_ON:
                    stateUpdate.put("flash", 1);
                    break;
                case CaptureRequest.CONTROL_AE_MODE_ON_ALWAYS_FLASH:
                    stateUpdate.put("flash", 2);
                    break;
            }

            switch (timerDelay) {
                case 0:
                    stateUpdate.put("timer", 0);
                    break;
                case 3000:
                    stateUpdate.put("timer", 1);
                    break;
                case 10000:
                    stateUpdate.put("timer", 2);
                    break;
            }

            Wearable.getCapabilityClient(this).getCapability("camera_message", CapabilityClient.FILTER_REACHABLE)
                    .addOnCompleteListener(new OnCompleteListener<CapabilityInfo>() {
                        @Override
                        public void onComplete(@NonNull Task<CapabilityInfo> task) {
                            if (task.isSuccessful()) {
                                for (Node n : task.getResult().getNodes()) {
                                    if (n.isNearby()) {
                                        try {
                                            Task<Integer> launchTask = wearableMessageClient.sendMessage(n.getId(), "/cameraaction/stateupdate", stateUpdate.toString().getBytes("UTF-8"));
                                        } catch (Exception e) {
                                            Log.wtf("STATE_UPDATE_MESSAGE", e);
                                        }

                                    }
                                }
                            }
                        }
                    });
        } catch (Exception e) {
            Log.wtf("STATE_UPDATE_MESSAGE", e);
        }
    }

    @Override
    public void finish() {
        sock.close();

        super.finish();
    }

    @Override
    public void onDestroy() {
        NotificationManagerCompat mgr = NotificationManagerCompat.from(this);
        mgr.cancel(0);

        unregisterReceiver(closeReceiver);

        currentlyRunning = false;

        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        if (captureTimer != null) {
            Capture(null);
        } else {
            finish();
        }
    }
}
