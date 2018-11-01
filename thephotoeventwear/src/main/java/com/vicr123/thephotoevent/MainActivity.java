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
import android.support.annotation.NonNull;
import android.support.wearable.activity.WearableActivity;
import android.view.View;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.tasks.Tasks;
import com.google.android.gms.wearable.CapabilityClient;
import com.google.android.gms.wearable.CapabilityInfo;
import com.google.android.gms.wearable.Node;
import com.google.android.gms.wearable.Wearable;

public class MainActivity extends WearableActivity {
    String launchMainNodeId = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Enables Always-on
        setAmbientEnabled();

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

    public void launchHandheldApp(View view) {
        if (launchMainNodeId == null) {

        } else {
            Task<Integer> launchTask = Wearable.getMessageClient(this).sendMessage(launchMainNodeId, "/launch", null);
        }
    }
}
