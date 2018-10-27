package com.vicr123.thephotoevent;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Point;
import android.hardware.camera2.CameraMetadata;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.View;

public class CameraHudView extends View {
    int afState;
    Point afPoint;
    boolean fixedFocus = false;
    float density;

    Paint focusCircleFocusingPaint, focusCircleFocusedPaint, focusCircleFailedPaint;

    public CameraHudView(Context context) {
        super(context);
        init();
    }

    public CameraHudView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public CameraHudView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    public CameraHudView(Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    private void init() {
        density = Resources.getSystem().getDisplayMetrics().density;

        focusCircleFocusingPaint = new Paint();
        focusCircleFocusingPaint.setStyle(Paint.Style.STROKE);
        focusCircleFocusingPaint.setStrokeWidth(2 * density);
        focusCircleFocusingPaint.setColor(0xFFFFFFFF);

        focusCircleFocusedPaint = new Paint();
        focusCircleFocusedPaint.setStyle(Paint.Style.STROKE);
        focusCircleFocusedPaint.setStrokeWidth(2 * density);
        focusCircleFocusedPaint.setColor(0xFF00FF00);

        focusCircleFailedPaint = new Paint();
        focusCircleFailedPaint.setStyle(Paint.Style.STROKE);
        focusCircleFailedPaint.setStrokeWidth(2 * density);
        focusCircleFailedPaint.setColor(0xFFFF0000);
    }

    public void setAfState(int afState) {
        this.afState = afState;
        invalidate();
    }

    public void setAfPoint(Point afPoint) {
        this.afPoint = afPoint;
        invalidate();
    }

    public void setFixedFocus(boolean fixedFocus) {
        this.fixedFocus = fixedFocus;
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (fixedFocus) {
            Paint p;
            switch (afState) {
                case CameraMetadata.CONTROL_AF_STATE_ACTIVE_SCAN:
                case CameraMetadata.CONTROL_AF_STATE_PASSIVE_SCAN:
                case CameraMetadata.CONTROL_AF_STATE_INACTIVE:
                    p = focusCircleFocusingPaint;
                    break;
                case CameraMetadata.CONTROL_AF_STATE_FOCUSED_LOCKED:
                case CameraMetadata.CONTROL_AF_STATE_PASSIVE_FOCUSED:
                    p = focusCircleFocusedPaint;
                    break;
                case CameraMetadata.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED:
                case CameraMetadata.CONTROL_AF_STATE_PASSIVE_UNFOCUSED:
                    p = focusCircleFailedPaint;
                    break;
                default:
                    p = focusCircleFocusingPaint;
            }
            canvas.drawCircle(afPoint.x, afPoint.y, 30 * density, p);
            //canvas.drawCircle(afPoint.x, afPoint.y, 100, focusCircleFocusingPaint);
        }
    }
}
