#include <iostream>
#include <cstdio>
#include <string>
#include "opencv2/opencv.hpp"

#define NUM_COMMAND_LINE_ARGS 1

int main(int argc, char** argv) {

    std::string filename;

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
    else {
        std::cout << "Opened successfully" << std::endl;
    }
}