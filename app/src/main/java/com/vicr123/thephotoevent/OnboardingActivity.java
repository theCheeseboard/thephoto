package com.vicr123.thephotoevent;

import android.animation.ArgbEvaluator;
import android.preference.PreferenceManager;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.TextView;

public class OnboardingActivity extends AppCompatActivity {

    /**
     * The {@link android.support.v4.view.PagerAdapter} that will provide
     * fragments for each of the sections. We use a
     * {@link FragmentPagerAdapter} derivative, which will keep every
     * loaded fragment in memory. If this becomes too memory intensive, it
     * may be best to switch to a
     * {@link android.support.v4.app.FragmentStatePagerAdapter}.
     */
    private SectionsPagerAdapter mSectionsPagerAdapter;

    /**
     * The {@link ViewPager} that will host the section contents.
     */
    private ViewPager mViewPager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_onboarding);

        // Create the adapter that will return a fragment for each of the three
        // primary sections of the activity.
        mSectionsPagerAdapter = new SectionsPagerAdapter(getSupportFragmentManager());

        // Set up the ViewPager with the sections adapter.
        mViewPager = findViewById(R.id.container);
        mViewPager.setAdapter(mSectionsPagerAdapter);

        final int[] colors = new int[] {
                ContextCompat.getColor(this, R.color.colorPrimary),
                ContextCompat.getColor(this, R.color.colorPrimaryDark),
                ContextCompat.getColor(this, R.color.colorPrimary)
        };

        mViewPager.addOnPageChangeListener(new ViewPager.OnPageChangeListener() {
            @Override
            public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
                mViewPager.setBackgroundColor((Integer) (new ArgbEvaluator()).evaluate(positionOffset, colors[position], colors[position == mSectionsPagerAdapter.getCount() - 1 ? position : position + 1]));
            }

            @Override
            public void onPageSelected(int position) {
                mViewPager.setBackgroundColor(colors[position]);

                if (position == mSectionsPagerAdapter.getCount() - 1) {
                    findViewById(R.id.onboarding_next_button).setVisibility(View.GONE);
                    findViewById(R.id.onboarding_finish_button).setVisibility(View.VISIBLE);
                    findViewById(R.id.onboarding_skip_button).setVisibility(View.GONE);
                } else {
                    findViewById(R.id.onboarding_next_button).setVisibility(View.VISIBLE);
                    findViewById(R.id.onboarding_finish_button).setVisibility(View.GONE);
                    findViewById(R.id.onboarding_skip_button).setVisibility(View.VISIBLE);
                }
            }

            @Override
            public void onPageScrollStateChanged(int state) {

            }
        });

    }

    public void nextButton(View view) {
        mViewPager.setCurrentItem(mViewPager.getCurrentItem() + 1);
    }

    public void skipButton(View view) {
        mViewPager.setCurrentItem(mSectionsPagerAdapter.getCount() - 1);
    }

    public void finishButton(View view) {
        finish();
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

            View rootView;

            switch (page) {
                case 1:
                    rootView = inflater.inflate(R.layout.fragment_onboarding_page1, container, false);
                    break;
                case 2:
                    rootView = inflater.inflate(R.layout.fragment_onboarding_page2, container, false);

                    EditText displayName = rootView.findViewById(R.id.onboarding_name_input);
                    displayName.setText(Globals.getDeviceName());
                    displayName.addTextChangedListener(new TextWatcher() {
                        @Override
                        public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

                        }

                        @Override
                        public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
                            //Set display name
                            PreferenceManager.getDefaultSharedPreferences(getContext()).edit().putString("display_name", charSequence.toString()).apply();
                        }

                        @Override
                        public void afterTextChanged(Editable editable) {

                        }
                    });
                    break;
                case 3:
                    rootView = inflater.inflate(R.layout.fragment_onboarding_page3, container, false);
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
            return 2;
        }
    }
}
