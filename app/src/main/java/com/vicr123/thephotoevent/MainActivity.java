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

import android.Manifest;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Build;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.support.v4.app.ActivityCompat;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.content.ContextCompat;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.PopupMenu;
import android.widget.TextView;

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
                    //Change page
                    context.pager.setCurrentItem(MainActivity.PAGE_ERROR);
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

        //Change page
        context.pager.setCurrentItem(MainActivity.PAGE_ERROR);

        context = null;
    }
}

public class MainActivity extends AppCompatActivity {
    ViewPager pager;
    SectionsPagerAdapter pagerAdapter;

    static final int PAGE_INITIAL = 0;
    static final int PAGE_CONNECTING = 1;
    static final int PAGE_ERROR = 2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //Open the onboarding activity if needed
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        if (!prefs.getBoolean("ONBOARDING_COMPLETE", false)) {
            Intent intent = new Intent(this, OnboardingActivity.class);
            startActivityForResult(intent, 0);
            prefs.edit().putBoolean("ONBOARDING_COMPLETE", true).apply();
        }

        pagerAdapter = new SectionsPagerAdapter(getSupportFragmentManager());
        pager = findViewById(R.id.main_view_pager);
        pager.setAdapter(pagerAdapter);
    }

    public void goButtonClicked(View view) {
        //Hide keyboard
        ((InputMethodManager) getSystemService(INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(pager.getWindowToken(), 0);

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

        //Check for a valid code
        String code = ((EditText) findViewById(R.id.codeText)).getText().toString();
        code = code.replace(" ", "");
        if (code.length() != 10) {
            pager.setCurrentItem(PAGE_ERROR);
            return;
        }

        //Change page
        pager.setCurrentItem(PAGE_CONNECTING);

        new ConnectionTask().execute(this, code);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantedResults) {
        ArrayList<String> perms = new ArrayList<>(Arrays.asList(permissions));
        if (perms.contains(Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            if (grantedResults[perms.indexOf(Manifest.permission.WRITE_EXTERNAL_STORAGE)] == PackageManager.PERMISSION_DENIED) {
                //Disable the keep copy checkbox
                ((CheckBox) findViewById(R.id.keepCopyCheckbox)).setChecked(false);
                return;
            }
        }

        if (perms.contains(Manifest.permission.CAMERA)) {
            if (grantedResults[perms.indexOf(Manifest.permission.CAMERA)] == PackageManager.PERMISSION_DENIED) {
                //Do nothing
                return;
            }
        }

        //If all permissions were granted, connect to the computer
        goButtonClicked(null);
    }

    public void openMore(View view) {
        final Context ctx = this;
        PopupMenu menu = new PopupMenu(this, findViewById(R.id.moreButton));
        menu.getMenuInflater().inflate(R.menu.activity_main_options, menu.getMenu());
        menu.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener() {
            @Override
            public boolean onMenuItemClick(MenuItem menuItem) {
                Intent i;
                switch (menuItem.getItemId()) {
                    case R.id.menu_main_options_settings:
                        i = new Intent(ctx, SettingsActivity.class);
                        break;
                    case R.id.menu_main_options_help:
                        i = new Intent(ctx, HelpActivity.class);
                        break;
                    default:
                        i = new Intent();
                }
                startActivity(i);
                return false;
            }
        });
        menu.show();
    }

    public void openWifiSettings(View view) {
        startActivity(new Intent(Settings.ACTION_WIFI_SETTINGS));
        pager.setCurrentItem(PAGE_INITIAL);
    }

    public void returnToConnectionScreen(View view) {
        pager.setCurrentItem(PAGE_INITIAL);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.activity_main_menu_settings: {
                Intent i = new Intent(this, SettingsActivity.class);
                this.startActivity(i);
            }
        }
        return false;
    }

    public static class PlaceholderFragment extends Fragment {
        /**
         * The fragment argument representing the section number for this
         * fragment.
         */
        private static final String ARG_SECTION_NUMBER = "section_number";

        public PlaceholderFragment() {
        }

        /**
         * Returns a new instance of this fragment for the given section
         * number.
         */
        public static PlaceholderFragment newInstance(int sectionNumber) {
            PlaceholderFragment fragment = new PlaceholderFragment();
            Bundle args = new Bundle();
            args.putInt(ARG_SECTION_NUMBER, sectionNumber);
            fragment.setArguments(args);
            return fragment;
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
            int page = getArguments().getInt(ARG_SECTION_NUMBER);

            final View rootView;

            switch (page) {
                case 1:
                    rootView = inflater.inflate(R.layout.fragment_main, container, false);

                    final EditText codeText = rootView.findViewById(R.id.codeText);
                    codeText.addTextChangedListener(new TextWatcher() {
                        @Override
                        public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

                        }

                        @Override
                        public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
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
                    codeText.setOnKeyListener(new View.OnKeyListener() {
                        @Override
                        public boolean onKey(View view, int i, KeyEvent keyEvent) {
                            if (keyEvent.getAction() == KeyEvent.ACTION_DOWN && i == KeyEvent.KEYCODE_ENTER) {
                                rootView.findViewById(R.id.goButton).performClick();
                                return true;
                            }
                            return false;
                        }
                    });
                    codeText.setSelection(codeText.getText().length());
                    break;
                case 2:
                    rootView = inflater.inflate(R.layout.fragment_main_connecting, container, false);
                    break;
                case 3:
                    rootView = inflater.inflate(R.layout.fragment_main_error, container, false);
                    break;
                default:
                    rootView = new View(getContext());
            }

            return rootView;
        }
    }

    /**
     * A {@link FragmentPagerAdapter} that returns a fragment corresponding to
     * one of the sections/tabs/pages.
     */
    public class SectionsPagerAdapter extends FragmentPagerAdapter {

        public SectionsPagerAdapter(FragmentManager fm) {
            super(fm);
        }

        @Override
        public Fragment getItem(int position) {
            // getItem is called to instantiate the fragment for the given page.
            // Return a PlaceholderFragment (defined as a static inner class below).
            return PlaceholderFragment.newInstance(position + 1);
        }

        @Override
        public int getCount() {
            return 3;
        }
    }
}
