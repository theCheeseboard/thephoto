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

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationManagerCompat;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.wearable.CapabilityClient;
import com.google.android.gms.wearable.CapabilityInfo;
import com.google.android.gms.wearable.MessageEvent;
import com.google.android.gms.wearable.Node;
import com.google.android.gms.wearable.Wearable;
import com.google.android.gms.wearable.WearableListenerService;

public class WearableService extends WearableListenerService {
    NotificationCompat.Builder notificationBuilder;
    String launchMainNodeId = null;

    public WearableService() {
        super();
    }

    @Override
    public void onCreate() {
        super.onCreate();

        //Set up connected notification
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            //Set up notification channel on Android Oreo
            NotificationChannel channel = new NotificationChannel("connected_notification", getString(R.string.connected_notification_channel_name), NotificationManager.IMPORTANCE_LOW);
            channel.setDescription(getString(R.string.connected_notification_channel_description));

            NotificationManager mgr = getSystemService(NotificationManager.class);
            mgr.createNotificationChannel(channel);
        }


        notificationBuilder = new NotificationCompat.Builder(this, "connected_notification")
                                .setContentTitle(getString(R.string.connected_notification_title))
                                .setContentText(getString(R.string.connected_notification_text))
                                .setSmallIcon(R.drawable.ic_notification_icon)
                                .setOngoing(true);
    }

    @Override
    public void onMessageReceived(MessageEvent messageEvent) {
        final Context ctx = this;
        if (messageEvent.getPath().equals("/capcheck")) {
            CapabilityClient cap = Wearable.getCapabilityClient(this);
            cap.getCapability("camera_active", CapabilityClient.FILTER_REACHABLE)
                    .addOnCompleteListener(new OnCompleteListener<CapabilityInfo>() {
                        @Override
                        public void onComplete(@NonNull Task<CapabilityInfo> task) {
                            launchMainNodeId = null;

                            if (task.isSuccessful()) {
                                for (Node n : task.getResult().getNodes()) {
                                    if (n.isNearby()) {
                                        launchMainNodeId = n.getId();
                                        break;
                                    }
                                }
                            }

                            if (launchMainNodeId == null) {
                                NotificationManagerCompat.from(ctx).cancel(0);
                            } else {
                                Intent intent = new Intent(ctx, CameraActivity.class);
                                intent.putExtra("NODE_ID", launchMainNodeId);
                                PendingIntent pendingIntent = PendingIntent.getActivity(ctx, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);

                                notificationBuilder.setContentIntent(pendingIntent);
                                NotificationManagerCompat.from(ctx).notify(0, notificationBuilder.build());
                            }
                        }
                    });

        }
    }
}
