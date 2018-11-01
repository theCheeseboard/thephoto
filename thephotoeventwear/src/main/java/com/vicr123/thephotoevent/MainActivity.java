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

import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.NotificationManagerCompat;
import android.support.wearable.activity.ConfirmationActivity;
import android.support.wearable.activity.WearableActivity;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.OnSuccessListener;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.Tasks;
import com.google.android.gms.wearable.CapabilityClient;
import com.google.android.gms.wearable.CapabilityInfo;
import com.google.android.gms.wearable.MessageClient;
import com.google.android.gms.wearable.MessageEvent;
import com.google.android.gms.wearable.Node;
import com.google.android.gms.wearable.Wearable;

import org.json.JSONObject;

public class MainActivity extends WearableActivity {
    String launchMainNodeId = null;
    MessageClient messageClient;
    MessageClient.OnMessageReceivedListener wearableMessageReceivedListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        messageClient = Wearable.getMessageClient(this);

        // Enables Always-on
        setAmbientEnabled();

        //Tweak UI
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        int inset = (int) (0.146467f * metrics.widthPixels);
        findViewById(R.id.containerLayout).setPadding(inset, inset, inset, inset);

        //Set up listeners
        wearableMessageReceivedListener = new MessageClient.OnMessageReceivedListener() {
            @Override
            public void onMessageReceived(@NonNull MessageEvent messageEvent) {
                if (messageEvent.getPath().equals("/capcheck")) {
                    checkCapabilities();
                }
            }
        };
        checkCapabilities();

        try {
            CapabilityClient cap = Wearable.getCapabilityClient(this);
            cap.getCapability("launch_handheld_app", CapabilityClient.FILTER_REACHABLE)
                    .addOnCompleteListener(new OnCompleteListener<CapabilityInfo>() {
                @Override
                public void onComplete(@NonNull Task<CapabilityInfo> task) {
                    if (task.isSuccessful()) {
                        for (Node n : task.getResult().getNodes()) {
                            if (n.isNearby()) {
                                launchMainNodeId = n.getId();
                                break;
                            }
                        }
                    }

                    if (launchMainNodeId == null) {
                        ((TextView) findViewById(R.id.explainText)).setText(R.string.get_on_play);
                    }
                }
            });
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        messageClient.addListener(wearableMessageReceivedListener, Uri.parse("wear://*/capcheck"), MessageClient.FILTER_PREFIX);
    }

    @Override
    public void onPause() {
        super.onPause();
        messageClient.removeListener(wearableMessageReceivedListener);
    }

    @Override
    public void onEnterAmbient(Bundle ambientDetails) {
        super.onEnterAmbient(ambientDetails);

        findViewById(R.id.mainLayoutRootView).setBackgroundColor(Color.BLACK);
    }

    @Override
    public void onExitAmbient() {
        super.onExitAmbient();

        findViewById(R.id.mainLayoutRootView).setBackgroundResource(R.color.dark_grey);
    }

    private void checkCapabilities() {
        final Context ctx = this;
        CapabilityClient cap = Wearable.getCapabilityClient(this);
        cap.getCapability("camera_active", CapabilityClient.FILTER_REACHABLE)
                .addOnCompleteListener(new OnCompleteListener<CapabilityInfo>() {
                    @Override
                    public void onComplete(@NonNull Task<CapabilityInfo> task) {
                        if (task.isSuccessful()) {
                            for (Node n : task.getResult().getNodes()) {
                                if (n.isNearby()) {
                                    //Open the camera activity
                                    Intent intent = new Intent(ctx, CameraActivity.class);
                                    intent.putExtra("NODE_ID", n.getId());
                                    startActivity(intent);
                                    break;
                                }
                            }
                        }
                    }
                });
    }

    public void launchHandheldApp(View view) {
        if (launchMainNodeId == null) {

        } else {
            final Context ctx = this;
            Task<Integer> launchTask = messageClient.sendMessage(launchMainNodeId, "/launch", null);
            launchTask.addOnSuccessListener(new OnSuccessListener<Integer>() {
                @Override
                public void onSuccess(Integer integer) {
                    Intent intent = new Intent(ctx, ConfirmationActivity.class);
                    intent.putExtra(ConfirmationActivity.EXTRA_ANIMATION_TYPE, ConfirmationActivity.OPEN_ON_PHONE_ANIMATION);
                    intent.putExtra(ConfirmationActivity.EXTRA_MESSAGE, getString(R.string.check_phone));
                    startActivity(intent);
                }
            });
        }
    }
}
