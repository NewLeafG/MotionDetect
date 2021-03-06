package com.muse.motiondetect;

import android.app.Activity;
import android.graphics.Point;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SubMenu;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.CvType;
import org.opencv.core.Mat;

import java.util.List;
import java.util.ListIterator;

public class MainActivity extends Activity implements CvCameraViewListener2 {

    public native String hello();

    private static final String    TAG = "OCVSample::Activity";

    private Mat mRgba;
    private Mat                    mIntermediateMat;
    private Mat                    mGray;

    private final int                    mResGroupId = 2;

    private List<Camera.Size> mResolutionList;
    private MenuItem[] mResolutionMenuItems;
    private SubMenu mResolutionMenu;

    private CamView mOpenCvCameraView;

    public native double[] FindMoving(long matAddrGr, long matAddrRgba);
    public native void SetColor(long matAddrRgba, int[] data);

    public MainActivity() {
        Log.i(TAG, "Instantiated new " + this.getClass());
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "called onCreate");
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_main);

        mOpenCvCameraView = (CamView) findViewById(R.id.cv_surface_view);
        mOpenCvCameraView.enableView();
        mOpenCvCameraView.setCvCameraViewListener(this);
        mOpenCvCameraView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {

                int cols = mRgba.cols();
                int rows = mRgba.rows();

//                int xOffset = (mOpenCvCameraView.getWidth() - cols) / 2;
//                int yOffset = (mOpenCvCameraView.getHeight() - rows) / 2;

//                int x = (int)event.getX() - xOffset;
//                int y = (int)event.getY() - yOffset;

                Point startPnt = mOpenCvCameraView.getStartPoint();
                int x = (int)event.getX() - startPnt.x;
                int y = (int)event.getY() - startPnt.y;
                float scale = mOpenCvCameraView.getScale();
                if(scale!=0){
                    x = (int)(x/scale);
                    y = (int)(y/scale);
                }

                Log.i(TAG, "Touch image coordinates: (" + x + ", " + y + ")");

                if ((x < 0) || (y < 0) || (x > cols) || (y > rows)) return false;

                int[] data = {x,y};
                SetColor(mRgba.getNativeObjAddr(),data);
//                Rect touchedRect = new Rect();
//
//                touchedRect.x = (x>4) ? x-4 : 0;
//                touchedRect.y = (y>4) ? y-4 : 0;
//
//                touchedRect.width = (x+4 < cols) ? x + 4 - touchedRect.x : cols - touchedRect.x;
//                touchedRect.height = (y+4 < rows) ? y + 4 - touchedRect.y : rows - touchedRect.y;
//
//                Mat touchedRegionRgba = mRgba.submat(touchedRect);
//
//                Mat touchedRegionHsv = new Mat();
//                Imgproc.cvtColor(touchedRegionRgba, touchedRegionHsv, Imgproc.COLOR_RGB2HSV_FULL);

                return false;
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        Log.i(TAG, "called onCreateOptionsMenu");
        //        mItemPreviewRGBA = menu.add("Preview RGBA");
        //        mItemPreviewGray = menu.add("Preview GRAY");
        //        mItemPreviewCanny = menu.add("Canny");
        //        mItemPreviewFeatures = menu.add("Find features");

        if(mOpenCvCameraView==null)return false;
        mResolutionMenu = menu.addSubMenu("Resolution");
        mResolutionList = mOpenCvCameraView.getResolutionList();
        mResolutionMenuItems = new MenuItem[mResolutionList.size()];

        ListIterator<Camera.Size> resolutionItr = mResolutionList.listIterator();
        int idx = 0;
        while(resolutionItr.hasNext()) {
            Camera.Size element = resolutionItr.next();
            mResolutionMenuItems[idx] = mResolutionMenu.add(mResGroupId, idx, Menu.NONE,
                    Integer.valueOf(element.width).toString() + "*" + Integer.valueOf(element.height).toString());
            idx++;
        }

        return true;
    }

    @Override
    public void onPause()
    {
        super.onPause();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    @Override
    public void onResume()
    {
        super.onResume();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.enableView();
    }

    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    public void onCameraViewStarted(int width, int height) {
        mResolutionList = mOpenCvCameraView.getResolutionList();
        Camera.Size resolution = mResolutionList.get(2);
        mOpenCvCameraView.setResolution(resolution);

        mRgba = new Mat(height, width, CvType.CV_8UC4);
        mIntermediateMat = new Mat(height, width, CvType.CV_8UC4);
        mGray = new Mat(height, width, CvType.CV_8UC1);
    }

    public void onCameraViewStopped() {
        mRgba.release();
        mGray.release();
        mIntermediateMat.release();
    }

    public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        mRgba = inputFrame.rgba();
        mGray = inputFrame.gray();
        double[] rlt=FindMoving(mGray.getNativeObjAddr(), mRgba.getNativeObjAddr());
        if(rlt!=null)
            Log.d(TAG,"POS:"+rlt[0]+rlt[1]);

        return mRgba;
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        Log.i(TAG, "called onOptionsItemSelected; selected item: " + item);

        if (item.getGroupId() == mResGroupId)
        {
            int id = item.getItemId();
            Camera.Size resolution = mResolutionList.get(id);
            mOpenCvCameraView.setResolution(resolution);
            resolution = mOpenCvCameraView.getResolution();
            String caption = Integer.valueOf(resolution.width).toString() + "*" + Integer.valueOf(resolution.height).toString();
            Toast.makeText(this, caption, Toast.LENGTH_SHORT).show();
        }

        return true;
    }

    static {
        Log.i(TAG,"Load OpenCV library !");
        if (!OpenCVLoader.initDebug()) {
            Log.i(TAG,"OpenCV load not successfully");
        } else {
            System.loadLibrary("motiondetect");// load other libraries
        }
    }
}
