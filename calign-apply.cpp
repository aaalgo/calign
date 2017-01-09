#include <iostream>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/progress.hpp>
#include <opencv2/opencv.hpp>
#include <glog/logging.h>
#include "calign.h"
#include "image-io.h"

using namespace std;
namespace fs = boost::filesystem;

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
    setup_dicom(argv[0]);

    calign::CAlign align;
    align.load(model_path.native());
    cv::Mat image = load_image(from_path.native());
    CHECK(image.isContinuous());
    align.apply(image.ptr<uint16_t>(0), image.total());
    cv::imwrite(to_path.native(), image);
    return 0;
}
