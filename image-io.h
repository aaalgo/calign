#ifndef AAALGO_IMAGE_IO
#define AAALGO_IMAGE_IO
#include <string>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <opencv2/opencv.hpp>

void setup_dicom (char *);
cv::Mat load_image (std::string const &path);


#endif

