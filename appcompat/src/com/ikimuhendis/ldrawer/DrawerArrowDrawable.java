package com.ikimuhendis.ldrawer;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;

import android.support.v7.appcompat.R;

public abstract class DrawerArrowDrawable extends Drawable {
    private static final float ARROW_HEAD_ANGLE = (float) Math.toRadians(45.0D);
    protected float mBarGap;
    protected float mBarSize;
    protected float mBarThickness;
    protected float mMiddleArrowSize;
    protected final Paint mPaint = new Paint();
    protected final Path mPath = new Path();
    protected float mProgress;
    protected int mSize;
    protected float mVerticalMirror = 1f;
    protected float mTopBottomArrowSize;
    protected Context context;

    public DrawerArrowDrawable(Context context) {
        this.context = context;
        this.mPaint.setAntiAlias(true);
        this.mPaint.setColor(context.getResources().getColor(R.color.ldrawer_color));
        this.mSize = context.getResources().getDimensionPixelSize(R.dimen.ldrawer_drawableSize);
        this.mBarSize = context.getResources().getDimensionPixelSize(R.dimen.ldrawer_barSize);
        this.mTopBottomArrowSize = context.getResources().getDimensionPixelSize(R.dimen.ldrawer_topBottomBarArrowSize);
        this.mBarThickness = context.getResources().getDimensionPixelSize(R.dimen.ldrawer_thickness);
        this.mBarGap = context.getResources().getDimensionPixelSize(R.dimen.ldrawer_gapBetweenBars);
        this.mMiddleArrowSize = context.getResources().getDimensionPixelSize(R.dimen.ldrawer_middleBarArrowSize);
        this.mPaint.setStyle(Paint.Style.STROKE);
        this.mPaint.setStrokeJoin(Paint.Join.ROUND);
        this.mPaint.setStrokeCap(Paint.Cap.SQUARE);
        this.mPaint.setStrokeWidth(this.mBarThickness);
    }

    protected float lerp(float paramFloat1, float paramFloat2, float paramFloat3) {
        return paramFloat1 + paramFloat3 * (paramFloat2 - paramFloat1);
    }

    public void draw(Canvas canvas) {
        Rect localRect = getBounds();
        float f1 = lerp(this.mBarSize, this.mTopBottomArrowSize, this.mProgress);
        float f2 = lerp(this.mBarSize, this.mMiddleArrowSize, this.mProgress);
        float f3 = lerp(0.0F, this.mBarThickness / 2.0F, this.mProgress);
        float f4 = lerp(0.0F, ARROW_HEAD_ANGLE, this.mProgress);
        float f5 = 0.0F;
        float f6 = 180.0F;
        float f7 = lerp(f5, f6, this.mProgress);
        float f8 = lerp(this.mBarGap + this.mBarThickness, 0.0F, this.mProgress);
        this.mPath.rewind();
        float f9 = -f2 / 2.0F;
        this.mPath.moveTo(f9 + f3, 0.0F);
        this.mPath.rLineTo(f2 - f3, 0.0F);
        float f10 = (float) Math.round(f1 * Math.cos(f4));
        float f11 = (float) Math.round(f1 * Math.sin(f4));
        this.mPath.moveTo(f9, f8);
        this.mPath.rLineTo(f10, f11);
        this.mPath.moveTo(f9, -f8);
        this.mPath.rLineTo(f10, -f11);
        this.mPath.moveTo(0.0F, 0.0F);
        this.mPath.close();
        canvas.save();
        if (!isLayoutRtl())
            canvas.rotate(180.0F, localRect.centerX(), localRect.centerY());
        canvas.rotate(f7 * mVerticalMirror, localRect.centerX(), localRect.centerY());
        canvas.translate(localRect.centerX(), localRect.centerY());
        canvas.drawPath(this.mPath, this.mPaint);
        canvas.restore();
    }

    public int getIntrinsicHeight() {
        return this.mSize;
    }

    public int getIntrinsicWidth() {
        return this.mSize;
    }

    public void setAlpha(int alpha) {
        this.mPaint.setAlpha(alpha);
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }

    public abstract boolean isLayoutRtl();

    public void setColorFilter(ColorFilter colorFilter) {
        this.mPaint.setColorFilter(colorFilter);
    }

    public void setVerticalMirror(boolean mVerticalMirror) {
        this.mVerticalMirror = mVerticalMirror ? 1 : -1;
    }

    public void setProgress(float paramFloat) {
        this.mProgress = paramFloat;
        invalidateSelf();
    }

    public void setColor(int resourceId) {
        this.mPaint.setColor(context.getResources().getColor(resourceId));
    }
}