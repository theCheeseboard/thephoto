package com.the.thephoto;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.drawable.GradientDrawable;
import android.hardware.Camera;
import android.hardware.SensorManager;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.StrictMode;
import android.provider.MediaStore;
import android.support.design.widget.Snackbar;
import android.support.v4.app.NotificationManagerCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.support.v7.app.NotificationCompat;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.Toast;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Timer;

public class CameraStreamActivity extends AppCompatActivity {
    CameraView camView = null;
    OrientationEventListener orientationListener;
    Context cx;
    Thread readThread;
    Boolean saveData;
    Notification runningNotification;
    BroadcastReceiver closeActivityReceiver;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        cx = this;

        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                e.printStackTrace();
                try {
                    vars.PcSock.getOutputStream().write("ENDSTREAM\r\n".getBytes());
                    vars.PcSock.close();
                } catch (Exception ex) {
                }
                readThread.interrupt();

                AlertDialog.Builder builder = new AlertDialog.Builder(cx);
                builder.setTitle("Internal Error");
                builder.setMessage("thePhoto has run into an internal error.\n\n" + e.getStackTrace());
                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        finish();
                    }
                });
                builder.show();
            }
        });

        saveData = getIntent().getBooleanExtra("saveData", false);

        StrictMode.ThreadPolicy.Builder policy = new StrictMode.ThreadPolicy.Builder();
        policy.permitAll();
        StrictMode.setThreadPolicy(policy.build());

        setContentView(R.layout.activity_camera_stream);

        orientationListener = new OrientationEventListener(this, SensorManager.SENSOR_DELAY_NORMAL) {
            @Override
            public void onOrientationChanged(int orient) {
                if (orient != OrientationEventListener.ORIENTATION_UNKNOWN) {
                    int orientation = 360 + (orient * -1);
                    findViewById(R.id.flashButton).setRotation(orientation);
                    findViewById(R.id.timerButton).setRotation(orientation);
                    findViewById(R.id.flipCameraButton).setRotation(orientation);

                    android.hardware.Camera.CameraInfo info = new android.hardware.Camera.CameraInfo();
                    android.hardware.Camera.getCameraInfo(0, info);
                    orient = (orient + 45) / 90 * 90;
                    int rotation = 0;
                    rotation = (info.orientation + orient) % 360;
                    camView.setRotation(rotation);

                }
            }
        };

        //getSupportActionBar().hide();
        findViewById(R.id.CapButton).setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    ((ImageButton) v).setImageResource(R.drawable.capturepress);
                } else if (event.getAction() == MotionEvent.ACTION_UP) {
                    ((ImageButton) v).setImageResource(R.drawable.capture);
                }
                return false;
            }
        });

        readThread = new Thread(new Runnable() {
            @Override
            public void run() {
                Looper.prepare();
                try {
                    InputStream sock = vars.PcSock.getInputStream();
                    while (true) {
                        if (sock.available() > 0) {
                            byte[] buf = new byte[2048];
                            int read = sock.read(buf);

                            String str = new String(buf, 0, read, "UTF-8");
                            if (str.startsWith("CONNECTIONS")) {
                                String numberConnection = str.substring(str.indexOf(" ") + 1).replace("\r\n", "");
                                if (numberConnection.equals("1")) {
                                    Snackbar.make(findViewById(R.id.CapButton), "You are the only connected device.", Snackbar.LENGTH_LONG).show();
                                } else {
                                    Snackbar.make(findViewById(R.id.CapButton), numberConnection + " devices are now connected.", Snackbar.LENGTH_LONG).show();
                                }
                            } else if (str.startsWith("OK")) {
                                new Handler(cx.getMainLooper()).post(new Runnable() {
                                    @Override
                                    public void run() {
                                        findViewById(R.id.CapButton).setVisibility(View.VISIBLE);
                                        findViewById(R.id.timerButton).setVisibility(View.VISIBLE);
                                        findViewById(R.id.flashButton).setVisibility(View.VISIBLE);
                                        //findViewById(R.id.flipCameraButton).setVisibility(View.VISIBLE);
                                        findViewById(R.id.sendingBar).setVisibility(View.INVISIBLE);

                                        camView.startPreview();
                                    }
                                });
                            } else if (str.startsWith("ENDSTREAM")) {
                                finish();
                            }
                        }
                        Thread.sleep(1000);
                    }
                } catch (Exception e) {

                }
            }
        });
        readThread.start();

        IntentFilter closeFilter = new IntentFilter("CLOSE_CAPTURE");
        closeActivityReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                finish();
            }
        };
        registerReceiver(closeActivityReceiver, closeFilter);

        NotificationCompat.Builder notificationBuilder = new NotificationCompat.Builder(cx);
        notificationBuilder.setSmallIcon(R.mipmap.ic_launcher);
        notificationBuilder.setContentTitle("thePhoto is running");
        notificationBuilder.setContentText("thePhoto is currently connected to a PC");
        notificationBuilder.setOngoing(true);
        notificationBuilder.setColor(ContextCompat.getColor(cx, R.color.colorPrimary));

        PendingIntent openNotificationIntent = PendingIntent.getActivity(cx, 0, new Intent(this, this.getClass()), 0);
        notificationBuilder.setContentIntent(openNotificationIntent);

        PendingIntent closeActivityPending = PendingIntent.getBroadcast(cx, 0, new Intent("CLOSE_CAPTURE"), 0);
        notificationBuilder.addAction(R.drawable.ic_close_black_24dp, "Disconnect", closeActivityPending);

        runningNotification = notificationBuilder.build();

        NotificationManager notificationManager = (NotificationManager) cx.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(0, runningNotification);
    }

    @Override
    protected void onStart() {
        super.onStart();

    }

    @Override
    protected void onPause() {
        ((FrameLayout) findViewById(R.id.CamView)).removeView(camView);
        camView.releaseCamera();
        orientationListener.disable();
        camView = null;
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        orientationListener.enable();

        if (camView == null) {
            camView = new CameraView(this);

            camView.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        Rect touchRect = new Rect((int) (event.getX() - event.getTouchMajor() / 2), (int) (event.getY() - event.getTouchMajor() / 2), (int) (event.getX() + event.getTouchMajor() / 2), (int) (event.getY() + event.getTouchMinor() / 2));
                        Rect focusArea = new Rect();

                        focusArea.set(touchRect.left * 2000 / v.getWidth() - 1000,
                                touchRect.top * 2000 / v.getHeight() - 1000,
                                touchRect.right * 2000 / v.getWidth() - 1000,
                                touchRect.bottom * 2000 / v.getHeight() - 1000);

                        Camera.Area a = new Camera.Area(focusArea, 1000);

                        boolean lock;
                        //if (event.getDownTime() > 2000) {
                        //    lock = true;
                        //    Toast.makeText(cx, "AE Lock enabled", Toast.LENGTH_SHORT).show();
                        //} else {
                        lock = false;
                        //}

                        if (event.getX() > 10 && event.getX() < camView.getWidth() - 10 &&
                                event.getY() > 10 && event.getY() < camView.getHeight() - 10) {
                            camView.setFocusPoint(a, event.getX(), event.getY(), false);
                        }
                    }
                    return false;
                }
            });

            camView.setOnPictureTaken(new CameraView.onPictureTakenListener() {
                @Override
                public void OnPictureTaken(byte[] data) {
                    final ImageView preview = (ImageView) findViewById(R.id.imagePreview);
                    Bitmap image = BitmapFactory.decodeByteArray(data, 0, data.length);

                    if (saveData) {
                        MediaStore.Images.Media.insertImage(cx.getContentResolver(), image, new SimpleDateFormat("ddMMyyhhmmss").format(Calendar.getInstance().getTime()) + ".png", "thephoto");
                    }

                    Matrix m = new Matrix();
                    //m.postRotate(90);
                    image = Bitmap.createBitmap(image, 0, 0, image.getWidth(), image.getHeight(), m, true);
                    preview.setImageBitmap(image);
                    preview.setVisibility(View.VISIBLE);
                    Animation FlyAnimation = AnimationUtils.loadAnimation(cx, R.anim.slide_to_left);
                    FlyAnimation.setAnimationListener(new Animation.AnimationListener() {
                        @Override
                        public void onAnimationStart(Animation animation) {

                        }

                        @Override
                        public void onAnimationEnd(Animation animation) {
                            preview.setVisibility(View.GONE);
                            animation.reset();
                        }

                        @Override
                        public void onAnimationRepeat(Animation animation) {

                        }
                    });
                    FlyAnimation.setRepeatMode(Animation.RELATIVE_TO_PARENT);
                    preview.startAnimation(FlyAnimation);
                }
            });
            ((FrameLayout) findViewById(R.id.CamView)).addView(camView);
        }
    }

    public void SwitchFlash (View view) {
        switch (camView.getFlash()) {
            case Camera.Parameters.FLASH_MODE_AUTO:
                camView.setFlash(Camera.Parameters.FLASH_MODE_OFF);
                ((ImageButton) findViewById(R.id.flashButton)).setImageResource(R.drawable.ic_flash_off_24dp);
                break;
            case Camera.Parameters.FLASH_MODE_OFF:
                camView.setFlash(Camera.Parameters.FLASH_MODE_ON);
                ((ImageButton) findViewById(R.id.flashButton)).setImageResource(R.drawable.ic_flash_on_24dp);
                break;
            case Camera.Parameters.FLASH_MODE_ON:
                camView.setFlash(Camera.Parameters.FLASH_MODE_AUTO);
                ((ImageButton) findViewById(R.id.flashButton)).setImageResource(R.drawable.ic_flash_auto_24dp);
                break;
        }
    }

    public void SwitchTimer (View view) {
        switch (camView.getTimer()) {
            case 0:
                camView.setTimer(1);
                ((ImageButton) findViewById(R.id.timerButton)).setImageResource(R.drawable.ic_timer_10_24dp);
                break;
            case 1:
                camView.setTimer(2);
                ((ImageButton) findViewById(R.id.timerButton)).setImageResource(R.drawable.ic_timer_3_24dp);
                break;
            case 2:
                camView.setTimer(0);
                ((ImageButton) findViewById(R.id.timerButton)).setImageResource(R.drawable.ic_timer_24dp);
                break;
        }
    }

    public void SwitchCamera(View view) {
        camView.SwitchCamera();
    }

    public void ChangeSettings(View view) {

    }

    public void CaptureImage(View view) {
        camView.capture();
        findViewById(R.id.CapButton).setVisibility(View.INVISIBLE);
        findViewById(R.id.timerButton).setVisibility(View.INVISIBLE);
        findViewById(R.id.flashButton).setVisibility(View.INVISIBLE);
        //findViewById(R.id.flipCameraButton).setVisibility(View.INVISIBLE);

        findViewById(R.id.sendingBar).setVisibility(View.VISIBLE);
    }

    public void zoomIn(View view) {
        camView.zoomIn();
    }

    public void zoomOut(View view) {
        camView.zooomOut();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        switch (event.getKeyCode()) {
            case KeyEvent.KEYCODE_VOLUME_UP:
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    camView.zoomIn();
                }
                return true;
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    camView.zooomOut();
                }
                return true;
            default:
                return super.dispatchKeyEvent(event);
        }
    }

    public void onStop() {
        super.onStop();
    }

    public void onDestroy() {
        super.onDestroy();
        try {
            vars.PcSock.getOutputStream().write("ENDSTREAM\r\n".getBytes());
            vars.PcSock.close();
        } catch (Exception e) {
        }
        readThread.interrupt();

        NotificationManager notificationManager = (NotificationManager) cx.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(0);

        unregisterReceiver(closeActivityReceiver);
    }
}
