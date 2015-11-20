#include <jni.h>
#include <opencv2/core/core.hpp>
//#include "./../OCV/include/opencv2/core/core.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <vector>
#include "com_MUSE_motiondetect_MainActivity.h"
#include <opencv2/video/tracking.hpp>
#include <opencv2/calib3d/calib3d_c.h>
#include <opencv2/calib3d.hpp>
//#include <utils/Log.h>
//#include <ALog.h>
#include <android/log.h>
#include <stdlib.h>

#define MEAN_THRESH 60.0 //9.0
/* Pre processing */
#define GAUSSIAN_SIZE 7 // Must be odd
#define MEDIAN_SIZE 3 // Must be odd

//static const int offset = yOffset;
using namespace std;
using namespace cv;

extern "C" {
//JNIEXPORT void JNICALL Java_com_emotiondetect_MainActivity_FindFeatures(JNIEnv*, jobject, jlong addrGray, jlong addrRgba);

static bool m_bInitiated = false;
cv::Mat m_prevImg;
cv::Mat m_nextImg;
std::vector<Point2f>   m_prevPts;
std::vector<cv::Point2f>   m_nextPts;
std::vector<unsigned char> m_status;
std::vector<float>         m_error;
static Size gb_size = Size(GAUSSIAN_SIZE, GAUSSIAN_SIZE);
static int m_itop = 0;
static int m_ileft = 0;
static int m_iright = 0;
static int m_ibottom = 0;
static int m_iCenX = 0;
static int m_iCenY = 0;
static int m_iPntCount = 0;
static Point m_pntCen(0,0);
static int m_left = 2000;
static int m_top = 2000;
static int m_right = 0;
static int m_bottom = 0;
bool mIsColorSelected = false;
Mat mPyrDownMat;
Mat mPyrDownMatGray;
Mat mHsvMat;
Mat mMask;
Mat mDilatedMask;
Mat mHierarchy;
vector<vector<Point> > mContours;
static double mMinContourArea = 0.1;

bool processColor(Mat rgbaImage);
//int dstWidth;
//int dstHeight;

// maxCorners šC The maximum number of corners to return. If there are more corners
// than that will be found, the strongest of them will be returned
int maxCorners = 100;
// qualityLevel šC Characterizes the minimal accepted quality of image corners;
// the value of the parameter is multiplied by the by the best corner quality
// measure (which is the min eigenvalue, see cornerMinEigenVal() ,
// or the Harris function response, see cornerHarris() ).
// The corners, which quality measure is less than the product, will be rejected.
// For example, if the best corner has the quality measure = 1500,
// and the qualityLevel=0.01 , then all the corners which quality measure is
// less than 15 will be rejected.
double qualityLevel = 0.01;
// minDistance šC The minimum possible Euclidean distance between the returned corners
double minDistance = 25;
// mask šC The optional region of interest. If the image is not empty (then it
// needs to have the type CV_8UC1 and the same size as image ), it will specify
// the region in which the corners are detected
cv::Mat mask;
// blockSize šC Size of the averaging block for computing derivative covariation
// matrix over each pixel neighborhood, see cornerEigenValsAndVecs()
int blockSize = 6;
// useHarrisDetector šC Indicates, whether to use operator or cornerMinEigenVal()
bool useHarrisDetector = true;
// k šC Free parameter of Harris detector
double k = 0.04;

static double m_dRlt[3];
Scalar mLowerBound(0);// = new Scalar(0);
Scalar mUpperBound(0);// = new Scalar(0);
Scalar mColorRadius(25,50,50,0);

void setHsvColor(Scalar hsvColor) {
    double minH = (hsvColor.val[0] >= mColorRadius.val[0]) ? hsvColor.val[0]-mColorRadius.val[0] : 0;
    double maxH = (hsvColor.val[0]+mColorRadius.val[0] <= 255) ? hsvColor.val[0]+mColorRadius.val[0] : 255;

    mLowerBound.val[0] = minH;
    mUpperBound.val[0] = maxH;

    mLowerBound.val[1] = hsvColor.val[1] - mColorRadius.val[1];
    mUpperBound.val[1] = hsvColor.val[1] + mColorRadius.val[1];

    mLowerBound.val[2] = hsvColor.val[2] - mColorRadius.val[2];
    mUpperBound.val[2] = hsvColor.val[2] + mColorRadius.val[2];

    mLowerBound.val[3] = 0;
    mUpperBound.val[3] = 255;
}

JNIEXPORT jstring JNICALL Java_com_muse_motiondetect_MainActivity_hello
        (JNIEnv * env, jobject obj){

    Mat mat;
    return (env)->NewStringUTF("Hello from as_JNI_OPENCV_PART");
}

// TODO: Warning: This method may be return a NULL value.
JNIEXPORT jdoubleArray JNICALL Java_com_muse_motiondetect_MainActivity_FindMoving
        (JNIEnv *env, jobject obj, jlong addrGray, jlong addrRgba){

    if(!mIsColorSelected)
        return NULL;

    pyrDown(*(Mat*)addrGray, mPyrDownMatGray);
    pyrDown(mPyrDownMatGray, mPyrDownMatGray);
    GaussianBlur(mPyrDownMatGray, mPyrDownMatGray, gb_size, 0, 0);
    medianBlur(mPyrDownMatGray, mPyrDownMatGray, MEDIAN_SIZE);
    if(processColor(*(Mat*)addrRgba)){
        m_dRlt[0] = m_pntCen.x;
        m_dRlt[1] = m_pntCen.y;
        m_dRlt[2] = 0;
        char coordinate[30];
        sprintf(coordinate,"(%d,%d,%d)",m_pntCen.x,m_pntCen.y,0);
        putText(*(Mat*)addrRgba, coordinate, m_pntCen, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255,0,255,0), 1, 8, false );

        jdoubleArray rlt = env->NewDoubleArray(3);
        env->SetDoubleArrayRegion(rlt,0,3,m_dRlt);
        return rlt;
    }

    if(m_bInitiated){

        m_nextImg = mPyrDownMatGray;
        if(m_prevImg.cols!=m_nextImg.cols){
            m_prevImg = m_nextImg.clone();
            return NULL;
        }
        cv::goodFeaturesToTrack(m_prevImg, m_prevPts, maxCorners, qualityLevel, minDistance, mask, blockSize, useHarrisDetector, k);

        if(m_prevPts.size() == 0){
            m_prevImg = m_nextImg.clone();
            return NULL;
        }

        for( unsigned int i = 0; i < m_prevPts.size(); i++ )
        {
            const Point2f& pt = m_prevPts[i];
            circle(*(Mat*)addrRgba, Point(pt.x * 4, pt.y * 4), 5, Scalar(255,0,0,255));
        }
        cv::calcOpticalFlowPyrLK(m_prevImg, m_nextImg, m_prevPts, m_nextPts, m_status, m_error, Size(10, 10), 3);

        std::vector <Point2f> prev_corner2, cur_corner2;
        // weed out bad matches
        //        int cornersize=0;
        for (size_t i = 0; i < m_status.size(); i++) {
            if (m_status[i]) {
                prev_corner2.push_back(m_prevPts[i]);
                cur_corner2.push_back(m_nextPts[i]);
                //                cornersize++;
            }
        }
        if(prev_corner2.size()<4){
            m_prevImg = m_nextImg.clone();
            return NULL;
        }
        cv::Mat H = cv::findHomography(prev_corner2, cur_corner2, CV_RANSAC);
        cv::Mat warp_frame;// = m_nextImg.clone();
        warpPerspective(m_nextImg.clone(), warp_frame, H, m_prevImg.size(), INTER_LINEAR | WARP_INVERSE_MAP);
        std::vector<Point2f> obj_corners(4);
        obj_corners[0] = cvPoint(0,0);
        obj_corners[1] = cvPoint( m_prevImg.cols, 0 );
        obj_corners[2] = cvPoint( m_prevImg.cols, m_prevImg.rows );
        obj_corners[3] = cvPoint( 0, m_prevImg.rows );
        std::vector<Point2f> scene_corners(4);
        perspectiveTransform( obj_corners, scene_corners, H );
        // m_ileft m_itop m_iright m_ibottom 应用仿射变换后的rect
        m_ileft = ceil(scene_corners[0].x<scene_corners[3].x?(0-scene_corners[0].x):(0-scene_corners[3].x));
        m_itop = ceil(scene_corners[0].y<scene_corners[1].y?(0-scene_corners[0].y):(0-scene_corners[1].y));
        m_iright = floor(scene_corners[1].x>scene_corners[2].x?m_prevImg.cols*2-scene_corners[1].x:m_prevImg.cols*2-scene_corners[2].x);
        m_ibottom = floor(scene_corners[2].y>scene_corners[3].y?m_prevImg.rows*2-scene_corners[2].y:m_prevImg.rows*2-scene_corners[3].y)-1;
        //		__android_log_print(ANDROID_LOG_DEBUG,"eMotionDetect"," scene_corners[2]:%f,%f",m_iright,scene_corners[2].y);
        m_iCenX = 0;
        m_iCenY = 0;
        m_iPntCount = 0;
        m_left = 2000;
        m_top = 2000;
        m_right = 0;
        m_bottom = 0;
        for(int row=0;row<m_prevImg.rows;row++){
            for(int col=0;col<m_prevImg.cols;col++){
                cv::Scalar pre_sclr = m_prevImg.at<uchar>(row, col);
                cv::Scalar nxt_sclr = warp_frame.at<uchar>(row, col);
                if(abs(pre_sclr[0]-nxt_sclr[0])<MEAN_THRESH){
                    //                    bin_mat->at<uchar>(row, col) = 0;
                    //        			circle(*(Mat*)addrRgba, Point(row, col), 5, Scalar(255,255,0,0));
                }
                else {
                    //foreground
                    //                    bin_mat->at<uchar>(row, col) = 255;
                    if(col>m_ileft&&col<m_iright&&row>m_itop&&row<m_ibottom){
                        circle(*(Mat*)addrRgba, Point(col * 4, row * 4), 2, Scalar(255,255,0,0));
                        m_iCenX += col * 4;
                        m_iCenY += row * 4;

                        if(col<m_left)m_left=col;
                        if(col>m_right)m_right=col;
                        if(row<m_top)m_top=row;
                        if(row>m_bottom)m_bottom=row;

                        m_iPntCount++;
                    }
                }
            }
        }
        if(m_iPntCount!=0){
            m_pntCen.x=m_iCenX/m_iPntCount;
            m_pntCen.y=m_iCenY/m_iPntCount;
            m_dRlt[0] = m_pntCen.x;
            m_dRlt[1] = m_pntCen.y;
            m_dRlt[2] = (m_right-m_left)*(m_bottom-m_top) * 4;
            char coordinate[30];
            sprintf(coordinate,"(%d,%d,%d)",m_pntCen.x,m_pntCen.y,(int)m_dRlt[2]);
            putText(*(Mat*)addrRgba, coordinate, m_pntCen, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255,0,255,0), 1, 8, false );

            rectangle(*(Mat*)addrRgba,Point(m_left * 4,m_top * 4),Point(m_right * 4,m_bottom * 4),Scalar(255,255,255));
        }
        m_prevImg = m_nextImg.clone();

    }else{
        m_prevImg = mPyrDownMatGray;
        //        int dstWidth = m_prevImg.cols;
        //        int dstHeight = m_prevImg.rows;
        m_bInitiated = true;
    }

    jdoubleArray rlt = env->NewDoubleArray(3);
    env->SetDoubleArrayRegion(rlt,0,3,m_dRlt);
    return rlt;
}

JNIEXPORT void JNICALL Java_com_muse_motiondetect_MainActivity_SetColor
        (JNIEnv *env, jobject obj, jlong addrRgba, jintArray data){
    // TODO set color to detect
    int* idata = env->GetIntArrayElements(data, NULL);

//    __android_log_print(ANDROID_LOG_DEBUG,"eMotionDetect"," coordinateData:%d,%d",idata[0], idata[1]);

    int x = idata[0];
    int y = idata[1];
    cv::Mat mRgba = *(Mat*)addrRgba;
    int cols = mRgba.cols;
    int rows = mRgba.rows;

    Rect touchedRect;// = new Rect();

    touchedRect.x = (x>4) ? x-4 : 0;
    touchedRect.y = (y>4) ? y-4 : 0;

    touchedRect.width = (x+4 < cols) ? x + 4 - touchedRect.x : cols - touchedRect.x;
    touchedRect.height = (y+4 < rows) ? y + 4 - touchedRect.y : rows - touchedRect.y;

    Mat touchedRegionRgba = mRgba(touchedRect);

    Mat touchedRegionHsv;// = new Mat();
    cvtColor(touchedRegionRgba, touchedRegionHsv, COLOR_RGB2HSV_FULL);

    // Calculate average color of touched region
    Scalar mBlobColorHsv = sum(touchedRegionHsv);
    int pointCount = touchedRect.width*touchedRect.height;
    for (int i = 0; i < 4; i++)
        mBlobColorHsv.val[i] /= pointCount;

    setHsvColor(mBlobColorHsv);
    mIsColorSelected = true;
}

bool processColor(Mat rgbaImage) {
    pyrDown(rgbaImage, mPyrDownMat);
    pyrDown(mPyrDownMat, mPyrDownMat);

    cvtColor(mPyrDownMat, mHsvMat, COLOR_RGB2HSV_FULL);

    inRange(mHsvMat, mLowerBound, mUpperBound, mMask);
    Mat temp;
    dilate(mMask, mDilatedMask, temp);

    vector<vector<Point> > contours;// = new ArrayList<MatOfPoint>();

    findContours(mDilatedMask, contours, mHierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    if(contours.size()==0)return false;

    // Find max contour area
    vector<Point> maxAreaContour;
    double maxArea = 0;
    for( int i = 0; i< contours.size(); i++ ) {
        double area = contourArea(contours[i]);
        if (area > maxArea)
            maxAreaContour = contours[i];
    }

    // Filter contours by area and resize to fit the original image size
    mContours.clear();
//    for( int i = 0; i< contours.size(); i++ ) {
//        if (contourArea(contours[i]) > mMinContourArea*maxArea) {
//            Scalar scalarH(4,4,0,0);
//            multiply(contours[i], scalarH, contours[i]);
//            mContours.push_back(contours[i]);
//        }
//    }
    Moments mu = moments(maxAreaContour, false);
//    Point2f( mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00 );
    m_pntCen.x = (int)((mu.m10/mu.m00)*4);
    m_pntCen.y = (int)((mu.m01/mu.m00)*4);

    multiply(maxAreaContour,Scalar(4,4,0,0),maxAreaContour);
    mContours.push_back(maxAreaContour);
    drawContours(rgbaImage, mContours, -1, Scalar(255,0,0,255));

    return true;
}

}


//-classpath /home/apollo/Android/Sdk/platforms/android-17/android.jar -o ${project_loc}/jni/JNI.h

//-verbose -classpath ../bin/classes;/home/apollo/Android/Sdk/platforms/android-17/android.jar -o ${project_loc}/jni/JNI.h ${java_type_name}


