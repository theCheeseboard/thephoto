package com.the.thephoto;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.MenuItem;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;

public class ConnectionActivity extends AppCompatActivity {

    Context cx;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_connection);

        getSupportActionBar().setTitle("Live Capture");
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        cx = this;
    }

    public void ConnectToPc(View view) {
        //Check that we have the camera permission before connecting
        int CameraPermissionCheck = ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA);

        if (CameraPermissionCheck != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, 0);
            return;
        }

        if (((CheckBox) findViewById(R.id.keepCopyCheck)).isChecked()) {
            int storagePermissionCheck = ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE);

            if (storagePermissionCheck != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
                return;
            }
        }

        findViewById(R.id.progressBar).setVisibility(View.VISIBLE);
        findViewById(R.id.textView2).setVisibility(View.VISIBLE);
        findViewById(R.id.button).setEnabled(false);
        final String IPAddress = ((EditText) findViewById(R.id.editText)).getText().toString();

        AsyncTask t = new AsyncTask() {
            @Override
            protected Object doInBackground(Object[] params) {
                try {
                    Socket sock = new Socket();
                    sock.connect(new InetSocketAddress(InetAddress.getByName(IPAddress), 26157), 20000);
                    vars.PcSock = sock;
                } catch (Exception e) {
                    e.printStackTrace();
                    this.cancel(true);
                }
                return null;
            }

            @Override
            protected void onPostExecute(Object o) {
                super.onPostExecute(o);
                Intent i = new Intent(cx, CameraStreamActivity.class);
                i.putExtra("saveData", ((CheckBox) findViewById(R.id.keepCopyCheck)).isChecked());
                finish();
                startActivity(i);
            }

            @Override
            protected void onCancelled() {
                super.onCancelled();
                findViewById(R.id.progressBar).setVisibility(View.INVISIBLE);
                findViewById(R.id.textView2).setVisibility(View.INVISIBLE);
                findViewById(R.id.button).setEnabled(true);

                AlertDialog.Builder d = new AlertDialog.Builder(cx);
                d.setTitle("Connection failed");
                d.setMessage("We couldn't connect to your PC.");
                d.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                    }
                });
                d.show();
            }
        };
        t.execute();

    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case 0:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    ConnectToPc(null);
                } else {
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("Permission Denied");
                    builder.setMessage("thePhoto doesn't have permission to access the camera.");
                    builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                        }
                    });
                    builder.show();
                }
                return;
            case 1:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    ConnectToPc(null);
                } else {
                    AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("Permission Denied");
                    builder.setMessage("thePhoto doesn't have permission to keep a copy of your images.");
                    builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            dialog.dismiss();
                        }
                    });
                    builder.show();
                }
                return;
        }
    }
}
