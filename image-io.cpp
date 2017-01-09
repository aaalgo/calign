#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <glog/logging.h>
#include "image-io.h"

namespace fs = boost::filesystem;
extern char _binary_dicom_dic_start;
extern char _binary_dicom_dic_end;

void load_dicom (std::string const &path, cv::Mat *image) {
    DicomImage *dcm = new DicomImage(path.c_str());
    CHECK(dcm) << "fail to new DicomImage";
    CHECK(dcm->getStatus() == EIS_Normal) << "fail to load dcm image";
    CHECK(dcm->isMonochrome()) << " only monochrome data supported.";
    auto depth = dcm->getDepth();
    LOG(INFO) << "depth: " << depth;
    CHECK((depth <= 16) || (depth > 8)) << " only 12/16-bit data supported but " << dcm->getDepth() << " found.";
    CHECK(dcm->getFrameCount() == 1) << " only single-framed dcm supported.";
    cv::Mat raw(dcm->getHeight(), dcm->getWidth(), CV_16U);
    dcm->getOutputData(raw.ptr<uint16_t>(0), raw.total() * sizeof(uint16_t), 16);
    delete dcm;
    *image = raw;
}

cv::Mat load_image (std::string const &path) {
    cv::Mat im = cv::imread(path, -1);
    if (im.total() == 0) {
        load_dicom(path, &im);
    }
    CHECK(im.total());
    CHECK(im.type() == CV_16U);
    CHECK(im.isContinuous());
    return im;
}


void setup_dicom (char *argv0) {
    char *env = getenv("DCMDICTPATH");
    if (env && strlen(env)) {
        if (fs::exists(env)) {
            LOG(INFO) << "using env DCMDICTPATH";
            return;
        }
        LOG(WARNING) << "DCMDICTPATH " << env << " not found";
    }
    fs::path home_dir = fs::path(argv0).parent_path();
    fs::path def = home_dir / fs::path("dicom.dic");
    if (!fs::exists(def)) {
        LOG(WARNING) << "Generating dicom.dic";
        fs::ofstream os(def);
        os.write(&_binary_dicom_dic_start, (&_binary_dicom_dic_end-&_binary_dicom_dic_start));
    }
    setenv("DCMDICTPATH", def.native().c_str(), 1);
}
