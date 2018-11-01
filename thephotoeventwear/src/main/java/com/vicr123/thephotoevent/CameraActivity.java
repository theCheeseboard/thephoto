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
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.constraint.ConstraintLayout;
import android.support.v4.view.ViewPager;
import android.support.wearable.activity.WearableActivity;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.wearable.CapabilityClient;
import com.google.android.gms.wearable.CapabilityInfo;
import com.google.android.gms.wearable.MessageClient;
import com.google.android.gms.wearable.MessageEvent;
import com.google.android.gms.wearable.Node;
import com.google.android.gms.wearable.Wearable;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

import in.goodiebag.carouselpicker.CarouselPicker;

public class CameraActivity extends WearableActivity {
    String nodeId;
    MessageClient messageClient;
    CarouselPicker timerPicker, flashPicker;
    MessageClient.OnMessageReceivedListener wearableMessageReceivedListener, capCheckMessageReceivedListener;
    boolean updatingState = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);

        nodeId = getIntent().getStringExtra("NODE_ID");
        messageClient = Wearable.getMessageClient(this);
        flashPicker = findViewById(R.id.flashPicker);
        timerPicker = findViewById(R.id.timerPicker);

        // Enables Always-on
        setAmbientEnabled();

        //Tweak UI
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);

        int captureSize = (int) (metrics.widthPixels - 64 * metrics.density);
        ConstraintLayout.LayoutParams layoutParams = new ConstraintLayout.LayoutParams(captureSize, captureSize);
        layoutParams.topMargin = (int) (32 * metrics.density);
        layoutParams.topToTop = ConstraintLayout.LayoutParams.PARENT_ID;
        layoutParams.leftToLeft = ConstraintLayout.LayoutParams.PARENT_ID;
        layoutParams.rightToRight = ConstraintLayout.LayoutParams.PARENT_ID;

        findViewById(R.id.captureButton).setLayoutParams(layoutParams);

        ViewPager.OnPageChangeListener listener = new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(int i, float v, int i1) {

            }

            @Override
            public void onPageSelected(int i) {
                if (!updatingState) {
                    sendStateUpdateMessage();
                }
            }

            @Override
            public void onPageScrollStateChanged(int i) {

            }
        };

        final Context ctx = this;

        //Add items to carousels
        List<CarouselPicker.PickerItem> flashItems = new ArrayList<>();
        flashItems.add(new CarouselPicker.DrawableItem(R.drawable.flashauto_default));
        flashItems.add(new CarouselPicker.DrawableItem(R.drawable.flashoff_default));
        flashItems.add(new CarouselPicker.DrawableItem(R.drawable.flashon_default));
        CarouselPicker.CarouselViewAdapter flashAdapter = new CarouselPicker.CarouselViewAdapter(this, flashItems, 0);
        flashPicker.setAdapter(flashAdapter);

        List<CarouselPicker.PickerItem> timerItems = new ArrayList<>();
        timerItems.add(new CarouselPicker.DrawableItem(R.drawable.timer0_default));
        timerItems.add(new CarouselPicker.DrawableItem(R.drawable.timer3_default));
        timerItems.add(new CarouselPicker.DrawableItem(R.drawable.timer10_default));
        CarouselPicker.CarouselViewAdapter timerAdapter = new CarouselPicker.CarouselViewAdapter(this, timerItems, 0);
        timerPicker.setAdapter(timerAdapter);

        flashPicker.addOnPageChangeListener(listener);
        timerPicker.addOnPageChangeListener(listener);

        //Set up message listeners
        wearableMessageReceivedListener = new MessageClient.OnMessageReceivedListener() {
            @Override
            public void onMessageReceived(@NonNull MessageEvent messageEvent) {
                if (messageEvent.getPath().equals("/cameraaction/stateupdate")) {
                    try {
                        JSONObject state = new JSONObject(new String(messageEvent.getData(), "UTF-8"));

                        updatingState = true;
                        flashPicker.setCurrentItem(state.getInt("flash"), true);
                        timerPicker.setCurrentItem(state.getInt("timer"), true);
                        updatingState = false;
                    } catch (Exception e) {
                        Log.wtf("wearable_message", e);
                    }
                }
            }
        };

        capCheckMessageReceivedListener = new MessageClient.OnMessageReceivedListener() {
            @Override
            public void onMessageReceived(@NonNull MessageEvent messageEvent) {
                if (messageEvent.getPath().equals("/capcheck")) {
                    CapabilityClient cap = Wearable.getCapabilityClient(ctx);
                    cap.getCapability("camera_active", CapabilityClient.FILTER_REACHABLE)
                            .addOnCompleteListener(new OnCompleteListener<CapabilityInfo>() {
                                @Override
                                public void onComplete(@NonNull Task<CapabilityInfo> task) {
                                    if (task.isSuccessful()) {
                                        for (Node n : task.getResult().getNodes()) {
                                            if (n.getId() == nodeId) return; //We're still connected so don't do anything
                                        }

                                        //We're done here
                                        finish();
                                    }
                                }
                            });
                }
            }
        };
    }

    @Override
    public void onResume() {
        super.onResume();

        Wearable.getCapabilityClient(this).addLocalCapability("camera_message");
        messageClient.addListener(wearableMessageReceivedListener, Uri.parse("wear://*/cameraaction"), MessageClient.FILTER_PREFIX);
        messageClient.addListener(capCheckMessageReceivedListener, Uri.parse("wear://*/capcheck"), MessageClient.FILTER_LITERAL);
        messageClient.sendMessage(nodeId, "/cameraaction/requeststate", null);
    }

    @Override
    public void onPause() {
        super.onPause();

        Wearable.getCapabilityClient(this).removeLocalCapability("camera_message");
        messageClient.removeListener(wearableMessageReceivedListener);
        messageClient.removeListener(capCheckMessageReceivedListener);
    }

    @Override
    public void onEnterAmbient(Bundle ambientDetails) {
        super.onEnterAmbient(ambientDetails);

        findViewById(R.id.cameraLayoutRootView).setBackgroundColor(Color.BLACK);
        findViewById(R.id.captureButton).setBackgroundResource(R.drawable.button_capture_ambient);
    }

    @Override
    public void onExitAmbient() {
        super.onExitAmbient();

        findViewById(R.id.cameraLayoutRootView).setBackgroundResource(R.color.dark_grey);
        findViewById(R.id.captureButton).setBackgroundResource(R.drawable.button_capture);
    }

    public void sendCaptureMessage(View view) {
        try {
            Task<Integer> launchTask = messageClient.sendMessage(nodeId, "/cameraaction", "CAPTURE".getBytes("UTF-8"));
        } catch (UnsupportedEncodingException e) {
            Log.wtf("WEARABLE_MESSAGE", e);
        }
    }

    public void sendStateUpdateMessage() {
        try {
            JSONObject stateUpdate = new JSONObject();
            stateUpdate.put("flash", flashPicker.getCurrentItem());
            stateUpdate.put("timer", timerPicker.getCurrentItem());

            Task<Integer> launchTask = messageClient.sendMessage(nodeId, "/cameraaction/stateupdate", stateUpdate.toString().getBytes("UTF-8"));
        } catch (Exception e) {
            Log.wtf("STATE_UPDATE_MESSAGE", e);
        }
    }
}
