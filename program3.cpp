#include <iostream>
#include <cstdio>
#include <string>
#include "opencv2/opencv.hpp"
#include <opencv2/tracking.hpp>

#define NUM_COMMAND_LINE_ARGS 1
#define MIN_CONTOUR_AREA 10000

int west_cars = 0; //36
int east_cars = 0; //14

void countEastTraffic(cv::Point& center_mass, int threshold, std::vector<cv::Point>& lane_pts);
void countWestTraffic(cv::Point& center_mass, int threshold, std::vector<cv::Point>& lane_pts);
void updateTrafficCountDisplay();

int main(int argc, char** argv) {

    std::string filename;

    cv::Scalar green(0,255,0);
    cv::Scalar red(0,0,255);

    // activation zones
    std::vector<cv::Point> east_left_lane_pts;
    std::vector<cv::Point> east_center_lane_pts;
    std::vector<cv::Point> west_left_lane_pts;
    std::vector<cv::Point> west_right_lane_pts;

    if(argc != NUM_COMMAND_LINE_ARGS + 1) {
        std::cout << "USAGE: " << argv[0] << "<filepath>" << std::endl;
        return 0;
    }
    else {
        filename = argv[1];
    }

    cv::VideoCapture capture(filename);

    if(!capture.isOpened()) {
        std::cout << "Unable to open video source, terminating program!" << std::endl;
        return 0;
    }
    
    std::cout << "Opened successfully" << std::endl;


    // ***begin processing***


    // mats
    cv::Mat frame, grayFrame, fgMask;

    // specs
    int captureWidth = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int captureHeight = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    int captureFPS = static_cast<int>(capture.get(cv::CAP_PROP_FPS));
    std::cout << captureWidth << " x " << captureHeight << std::endl;

    int resizedWidth = captureWidth/2;
    int resizedHeight = captureHeight/2;
    std::cout << "adjusted size: " << resizedWidth << " x " << resizedHeight << std::endl;

    cv::namedWindow("video", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("mask", cv::WINDOW_AUTOSIZE);


    // define sections
    // decipher east and west cars
    int directionThreshold = resizedHeight * 0.3;
    int leftBound_x = resizedWidth * 0.25;
    int rightBound_x = resizedWidth - leftBound_x;

    // ***East***
    // zones
    int LBoundL = leftBound_x - 30;
    int LBoundR = leftBound_x + 30;
    cv::Rect east_left_lane_zone(LBoundL, directionThreshold+60, 60, 100);
    cv::Rect east_middle_lane_zone(LBoundL, directionThreshold +170, 60, 110);

    // **West**
    // zones
    int RBoundL = rightBound_x - 30;
    int RBoundR = rightBound_x + 30;
    cv::Rect west_right_lane_zone(RBoundL, 0, 60, 60);
    cv::Rect west_left_lane_zone(RBoundL, 62, 60, 70);


    // configure background subtractor
    cv::Ptr<cv::BackgroundSubtractorMOG2> pMOG2;
    pMOG2 = cv::createBackgroundSubtractorMOG2();
    pMOG2->setDetectShadows(true);

    while(true) {

        bool success =capture.read(frame);

        if(!success) {
            break;
        }

        // resize frame before operations
        cv::resize(frame, frame, cv::Size(resizedWidth, resizedHeight));

        pMOG2->apply(frame, fgMask);

        cv::threshold(fgMask, fgMask, 240, 255, cv::THRESH_BINARY);
        cv::erode(fgMask, fgMask, cv::Mat(), cv::Point(-1,-1), 1);
        cv::dilate(fgMask, fgMask, cv::Mat(), cv::Point(-1,-1), 5);





        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(fgMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for(const auto& contour : contours) {
            cv::Rect rect = cv::boundingRect(contour);

            // filter out boundingRects that are too small
            if(rect.area() < 3000) {
                continue;
            }

            cv::Point centerMass(rect.x + rect.width/2, rect.y + rect.height/2);

            // east bound car
            if(centerMass.y > directionThreshold) {
                
                // we are within the left lane count zone // should be 20 cars here
                if(east_left_lane_zone.contains(centerMass)) {
                    countEastTraffic(centerMass, leftBound_x, east_left_lane_pts);
                }
                // we are within the middle lane count zone // should be 16 cars here
                else if(east_middle_lane_zone.contains(centerMass)) {
                    countEastTraffic(centerMass, leftBound_x, east_center_lane_pts);
                }

                cv::rectangle(frame, rect, red, 1);
            }
            else {

                // we are within the left lane count zone // should be 11
                if(west_left_lane_zone.contains(centerMass)) {
                    countWestTraffic(centerMass, rightBound_x, west_left_lane_pts);
                }
                // we are within the right lane count zone // should be 3 cars here
                else if(west_right_lane_zone.contains(centerMass)) {
                    countWestTraffic(centerMass, rightBound_x, west_right_lane_pts);
                }

                cv::rectangle(frame, rect, green, 1);
            }
        }


        cv::imshow("mask", fgMask);
        cv::imshow("video", frame);

        int delayMs = (1.0 / captureFPS) * 1000;
        if((char) cv::waitKey(delayMs) == 'q') {
            break;
        }
    }

    capture.release();
    cv::destroyAllWindows();
    return 0; 
}

void countEastTraffic(cv::Point& center_mass, int threshold, std::vector<cv::Point>& lane_pts) {
    // car's center of has not crossed threshold
    if(center_mass.x < threshold) {
        lane_pts.push_back(center_mass);
    }
    // a car that is passed the threshold and has already been counted should be ignored
    else if(center_mass.x > threshold && lane_pts.empty()) {
        return;
    }
    else {
        // grab the car's previous position
        cv::Point prev_center_mass = lane_pts.back();

        // if the car's previous position is behind the threshold and
        // its current position is past the threshold
        if(prev_center_mass.x < threshold && center_mass.x > threshold) {
            lane_pts.clear();

            east_cars++;
            updateTrafficCountDisplay();
        }
    }
}

// same principle as east bound function
void countWestTraffic(cv::Point& center_mass, int threshold, std::vector<cv::Point>& lane_pts) {

    if(center_mass.x > threshold) {
        lane_pts.push_back(center_mass);
    }
    else if(center_mass.x < threshold && lane_pts.empty() ) {
        return;
    }
    else {
        cv::Point prev_center_mass = lane_pts.back();

        if(prev_center_mass.x > threshold && center_mass.x < threshold) {
            lane_pts.clear();

            west_cars++;
            updateTrafficCountDisplay();
        }
    }
}

void updateTrafficCountDisplay() {
    std::cout << "WESTBOUND COUNT: " << west_cars << std::endl;
    std::cout << "EASTBOUND COUNT: " << east_cars << std::endl;
    std::cout << std::endl;
}