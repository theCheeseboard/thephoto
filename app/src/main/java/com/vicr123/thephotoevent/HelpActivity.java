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

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

public class HelpActivity extends AppCompatActivity {
    ViewPager pager;


    public static class HelpFragment extends Fragment {
        private static final String ARG_SECTION_NUMBER = "section_number";

        public HelpFragment() {
            // Required empty public constructor
        }

        // TODO: Rename and change types and number of parameters
        public static HelpFragment newInstance(int sectionNumber) {
            HelpFragment fragment = new HelpFragment();
            Bundle args = new Bundle();
            args.putInt(ARG_SECTION_NUMBER, sectionNumber);
            fragment.setArguments(args);
            return fragment;
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                                 Bundle savedInstanceState) {
            // Inflate the layout for this fragment
            View v;

            switch (getArguments().getInt(ARG_SECTION_NUMBER)) {
                case 1:
                    v = inflater.inflate(R.layout.fragment_help_main, container, false);
                    break;
                case 2:
                    v = inflater.inflate(R.layout.fragment_help_getting_started, container, false);
                    break;
                default:
                    v = new View(getContext());
            }

            return v;
        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_help);

        pager = findViewById(R.id.help_pager);
        pager.setAdapter(new FragmentPagerAdapter(getSupportFragmentManager()) {
            @Override
            public Fragment getItem(int position) {
                return HelpFragment.newInstance(position + 1);
            }

            @Override
            public int getCount() {
                return 2;
            }
        });


        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        switch (pager.getCurrentItem()) {
            case 0:
                //Nothing to go back to
                super.onBackPressed();
                break;
            default:
                //Go back to the main page
                pager.setCurrentItem(0);
        }
    }

    public void showGettingStartedPage(View view) {
        pager.setCurrentItem(1);
    }
}
