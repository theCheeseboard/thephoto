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

import android.os.Bundle;
import android.support.constraint.ConstraintLayout;
import android.support.wearable.activity.WearableActivity;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.google.android.gms.tasks.Task;
import com.google.android.gms.wearable.MessageClient;
import com.google.android.gms.wearable.Wearable;

import java.io.UnsupportedEncodingException;

public class CameraActivity extends WearableActivity {
    String nodeId;
    MessageClient messageClient;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_camera);

        nodeId = getIntent().getStringExtra("NODE_ID");
        messageClient = Wearable.getMessageClient(this);

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
    }

    public void sendCaptureMessage(View view) {
        try {
            Task<Integer> launchTask = messageClient.sendMessage(nodeId, "/cameraaction", "CAPTURE".getBytes("UTF-8"));
        } catch (UnsupportedEncodingException e) {
            Log.wtf("WEARABLE_MESSAGE", e);
        }
    }
}
