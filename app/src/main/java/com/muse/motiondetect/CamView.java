package com.muse.motiondetect;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Point;
import android.hardware.Camera.Size;
import android.util.AttributeSet;

import org.opencv.android.JavaCameraView;

import java.util.List;

public class CamView  extends JavaCameraView {

    private static final String TAG = "CamViewTag";

    public CamView(Context context, AttributeSet attrs){
        super(context, attrs);
    }

    public List<Size> getResolutionList() {
        return mCamera.getParameters().getSupportedPreviewSizes();
    }

    public void setResolution(Size resolution) {
        disconnectCamera();
        mMaxHeight = resolution.height;
        mMaxWidth = resolution.width;
        connectCamera(getWidth(), getHeight());
    }

    public Size getResolution() {
        return mCamera.getParameters().getPreviewSize();
    }

    public Point getStartPoint() {
        Canvas canvas = getHolder().lockCanvas();
        Point ret = new Point(0,0);
        if (canvas != null) {
            if (mScale != 0) {
//                canvas.drawBitmap(mCacheBitmap, new Rect(0, 0, mCacheBitmap.getWidth(), mCacheBitmap.getHeight()),
//                        new Rect((int) ((canvas.getWidth() - mScale * mCacheBitmap.getWidth()) / 2),
//                                (int) ((canvas.getHeight() - mScale * mCacheBitmap.getHeight()) / 2),
//                                (int) ((canvas.getWidth() - mScale * mCacheBitmap.getWidth()) / 2 + mScale * mCacheBitmap.getWidth()),
//                                (int) ((canvas.getHeight() - mScale * mCacheBitmap.getHeight()) / 2 + mScale * mCacheBitmap.getHeight())), null);
                ret = new Point((int) ((canvas.getWidth() - mScale * mFrameWidth) / 2),
                        (int) ((canvas.getHeight() - mScale * mFrameHeight) / 2));
            } else {
//                canvas.drawBitmap(mCacheBitmap, new Rect(0, 0, mCacheBitmap.getWidth(), mCacheBitmap.getHeight()),
//                        new Rect((canvas.getWidth() - mCacheBitmap.getWidth()) / 2,
//                                (canvas.getHeight() - mCacheBitmap.getHeight()) / 2,
//                                (canvas.getWidth() - mCacheBitmap.getWidth()) / 2 + mCacheBitmap.getWidth(),
//                                (canvas.getHeight() - mCacheBitmap.getHeight()) / 2 + mCacheBitmap.getHeight()), null);
                ret = new Point((canvas.getWidth() - mFrameWidth) / 2,
                        (canvas.getHeight() - mFrameHeight) / 2);
            }
            getHolder().unlockCanvasAndPost(canvas);
        }

        return ret;
    }

    public float getScale(){
        return mScale;
    }
}
