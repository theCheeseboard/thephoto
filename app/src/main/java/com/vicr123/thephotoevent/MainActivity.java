package com.vicr123.thephotoevent;

import android.Manifest;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;

import java.io.IOException;
import java.math.BigInteger;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import static com.vicr123.thephotoevent.Globals.sock;

class ConnectionTask extends AsyncTask<Object, Integer, Object> {
    MainActivity context;

    @Override
    protected Object doInBackground(Object... params) {
        context = (MainActivity) params[0];
        String code = (String) params[1];
        try {
            long ipNumber = Long.parseLong(code.substring(0, 10));

            byte[] ipBytes = new byte[] {
                    (byte) (ipNumber >> 24),
                    (byte) (ipNumber >> 16),
                    (byte) (ipNumber >> 8),
                    (byte) ipNumber
            };

            InetAddress addr = InetAddress.getByAddress(ipBytes);
            sock = new EventSocket(context);
            sock.setOnErrorHandler(new Runnable() {
                @Override
                public void run() {
                    //Error
                    new AlertDialog.Builder(context).setTitle(R.string.invalid_code_title)
                            .setMessage(R.string.invalid_code_message)
                            .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialogInterface, int i) {
                                    dialogInterface.dismiss();
                                }
                            }).show();

                    context.findViewById(R.id.connectingView).setVisibility(View.GONE);
                    context.findViewById(R.id.goButton).setEnabled(true);
                    context = null;
                }
            });
            sock.setOnEstablished(new Runnable() {
                @Override
                public void run() {
                    Intent i = new Intent(context, CameraActivity.class);
                    context.startActivity(i);

                    context.finish();
                }
            });
            sock.connect(new InetSocketAddress(addr, 26157), 5000);
        } catch (Exception e) {
            this.cancel(true);
            e.printStackTrace();
        }
        return null;
    }

    @Override
    protected void onPostExecute(Object o) {
        super.onPostExecute(o);
    }

    @Override
    protected void onCancelled() {
        super.onCancelled();

        //Error
        new AlertDialog.Builder(context).setTitle(R.string.invalid_code_title)
                .setMessage(R.string.invalid_code_message)
                .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        dialogInterface.dismiss();
                    }
                }).show();

        context.findViewById(R.id.connectingView).setVisibility(View.GONE);
        context.findViewById(R.id.goButton).setEnabled(true);
        context = null;
    }
}

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ((EditText) findViewById(R.id.codeText)).addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

            }

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                EditText codeText = findViewById(R.id.codeText);
                String text = charSequence.toString();
                int textLength = charSequence.length();

                if (charSequence.toString().endsWith(" ")) {
                    return;
                }

                if (textLength == 6)
                {
                    codeText.setText(new StringBuilder(text).insert(text.length() - 1, " ").toString());
                    codeText.setSelection(codeText.getText().length());
                }
            }

            @Override
            public void afterTextChanged(Editable editable) {

            }
        });
    }

    public void goButtonClicked(View view) {
        findViewById(R.id.connectingView).setVisibility(View.VISIBLE);
        findViewById(R.id.goButton).setEnabled(false);

        String code = ((EditText) findViewById(R.id.codeText)).getText().toString();
        code = code.replace(" ", "");
        if (code.length() != 10) {
            //Error
            new AlertDialog.Builder(this).setTitle(R.string.invalid_code_title)
                                     .setMessage(R.string.invalid_code_message)
                                     .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                                         @Override
                                         public void onClick(DialogInterface dialogInterface, int i) {
                                             dialogInterface.dismiss();
                                         }
                                     }).show();

            findViewById(R.id.connectingView).setVisibility(View.GONE);
            findViewById(R.id.goButton).setEnabled(true);
            return;
        }

        //Ensure permissions are granted
        ArrayList<String> permissions = new ArrayList<>();
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED) {
            //Permission needs to be requested
            permissions.add(Manifest.permission.CAMERA);
        }


        if (((CheckBox) findViewById(R.id.keepCopyCheckbox)).isChecked()) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_DENIED) {
                //Permission needs to be requested
                permissions.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
            }
        }

        if (permissions.size() != 0) {
            //Request permissions
            ActivityCompat.requestPermissions(this, permissions.toArray(new String[0]), 0);
            return;
        }

        new ConnectionTask().execute(this, code);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantedResults) {
        ArrayList<String> perms = new ArrayList<>(Arrays.asList(permissions));
        if (perms.contains(Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            if (grantedResults[perms.indexOf(Manifest.permission.WRITE_EXTERNAL_STORAGE)] == PackageManager.PERMISSION_DENIED) {
                //Disable the keep copy checkbox and return to let the user know
                ((CheckBox) findViewById(R.id.keepCopyCheckbox)).setChecked(false);

                findViewById(R.id.connectingView).setVisibility(View.GONE);
                findViewById(R.id.goButton).setEnabled(true);
                return;
            }
        }

        if (perms.contains(Manifest.permission.CAMERA)) {
            if (grantedResults[perms.indexOf(Manifest.permission.CAMERA)] == PackageManager.PERMISSION_DENIED) {
                //Return to let the user know

                findViewById(R.id.connectingView).setVisibility(View.GONE);
                findViewById(R.id.goButton).setEnabled(true);
                return;
            }
        }

        //If all permissions were granted, connect to the computer
        goButtonClicked(null);
    }
}
