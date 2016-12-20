package com.the.thephoto;

import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.drawable.GradientDrawable;
import android.hardware.Camera;
import android.hardware.SensorManager;
import android.os.Environment;
import android.os.Looper;
import android.os.StrictMode;
import android.provider.MediaStore;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
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
    CameraView camView;
    OrientationEventListener orientationListener;
    Context cx;
    Thread readThread;
    Boolean saveData;

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

                            String str = new String(buf, "UTF-8");
                            if (str.startsWith("CONNECTIONS")) {
                                String numberConnection = str.substring(str.indexOf(" ")).replace("\r\n", "");
                                if (numberConnection == "1") {
                                    Snackbar.make(findViewById(R.id.CapButton), "You are the only connected device.", Snackbar.LENGTH_LONG).show();
                                } else {
                                    Snackbar.make(findViewById(R.id.CapButton), numberConnection + " devices are now connected.", Snackbar.LENGTH_LONG).show();
                                }
                            }
                        }
                        Thread.sleep(1000);
                    }
                } catch (Exception e) {

                }
            }
        });
        readThread.start();
    }

    @Override
    protected void onStart() {
        super.onStart();
        orientationListener.enable();

    }

    @Override
    protected void onPause() {
        camView.releaseCamera();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        camView = new CameraView(this);

        camView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    Rect touchRect = new Rect((int)(event.getX() - event.getTouchMajor() / 2), (int)(event.getY() - event.getTouchMajor() / 2), (int)(event.getX() + event.getTouchMajor() / 2), (int)(event.getY() + event.getTouchMinor() / 2));
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
                    camView.setFocusPoint(a, event.getX(), event.getY(), false);
                }
                return false;


            }
        });

        camView.setOnPictureTaken(new CameraView.onPictureTakenListener() {
            @Override
            public void OnPictureTaken(byte[] data) {

                findViewById(R.id.CapButton).setVisibility(View.VISIBLE);
                findViewById(R.id.timerButton).setVisibility(View.VISIBLE);
                findViewById(R.id.flashButton).setVisibility(View.VISIBLE);
                findViewById(R.id.flipCameraButton).setVisibility(View.VISIBLE);
                final ImageView preview = (ImageView) findViewById(R.id.imagePreview);
                Bitmap image = BitmapFactory.decodeByteArray(data, 0, data.length);

                if (saveData) {
                    MediaStore.Images.Media.insertImage(cx.getContentResolver(), image, new SimpleDateFormat("ddMMyyhhmmss").format(Calendar.getInstance().getTime()) + ".png", "thephoto");
                }

                Matrix m = new Matrix();
                m.postRotate(90);
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
        SurfaceHolder holder = camView.getHolder();
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
        findViewById(R.id.flipCameraButton).setVisibility(View.INVISIBLE);
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
        try {
            vars.PcSock.getOutputStream().write("ENDSTREAM\r\n".getBytes());
            vars.PcSock.close();
        } catch (Exception e) {
        }
        readThread.interrupt();
        super.onStop();
    }
}
