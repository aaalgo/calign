#include <iostream>
#include <stdexcept>
#include <parallel/algorithm>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <opencv2/opencv.hpp>
#include <glog/logging.h>
#include "calign.h"

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


using namespace std;
namespace fs = boost::filesystem;
extern char _binary_dicom_dic_start;
extern char _binary_dicom_dic_end;

int main (int argc, char *argv[]) {
    fs::path from_path;
    fs::path to_path;
    fs::path model_path;
    {
        int verbose;
        namespace po = boost::program_options;
        po::options_description desc_visible("Allowed options");
        desc_visible.add_options()
            ("help,h", "produce help message.")
            ("from", po::value(&from_path), "")
            ("to", po::value(&to_path), "")
            ("model", po::value(&model_path), "")
            ;

        po::options_description desc("Allowed options");
        desc.add(desc_visible);

        po::positional_options_description p;
        p.add("model", 1);
        p.add("from", 1);
        p.add("to", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                         options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help") || from_path.empty() || to_path.empty() || model_path.empty()) {
            cout << "Usage:" << endl;
            cout << desc;
            cout << endl;
            return 0;
        }
    }
    do {   // setup dicom 
        char *env = getenv("DCMDICTPATH");
        if (env && strlen(env)) {
            if (fs::exists(env)) {
                LOG(INFO) << "using env DCMDICTPATH";
                break;
            }
            LOG(WARNING) << "DCMDICTPATH " << env << " not found";
        }
        fs::path home_dir = fs::path(argv[0]).parent_path();
        fs::path def = home_dir / fs::path("dicom.dic");
        if (!fs::exists(def)) {
            LOG(WARNING) << "Generating dicom.dic";
            fs::ofstream os(def);
            os.write(&_binary_dicom_dic_start, (&_binary_dicom_dic_end-&_binary_dicom_dic_start));
        }
        setenv("DCMDICTPATH", def.native().c_str(), 1);
    } while (false);

    calign::CAlign align;
    align.load(model_path.native());
    cv::Mat image = load_image(from_path.native());
    CHECK(image.isContinuous());
    align.apply(image.ptr<uint16_t>(0), image.total());
    cv::imwrite(to_path.native(), image);
    return 0;
}
