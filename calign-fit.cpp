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
    //LOG(INFO) << "depth: " << depth;
    CHECK((depth <= 16) || (depth > 8)) << " only 12/16-bit data supported but " << dcm->getDepth() << " found.";
    CHECK(dcm->getFrameCount() == 1) << " only single-framed dcm supported.";
    cv::Mat raw(dcm->getHeight(), dcm->getWidth(), CV_16U);
    dcm->getOutputData(raw.ptr<uint16_t>(0), raw.total() * sizeof(uint16_t), 16);
    if (depth == 12) {
        raw *= 16;
    }
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


namespace calign {
    using namespace std;

    class Histogram: public vector<uint16_t> {
    public:
        Histogram (vector<string> const &files, size_t max_image = 100000) {
            reserve(files.size() * max_image);
            boost::progress_display progress(files.size(), cout);
#pragma omp parallel
            {
                std::default_random_engine rng;
#pragma omp for
                for (unsigned i = 0; i < files.size(); ++i) {
                    cv::Mat im = load_image(files[i]);
                    //cout << path << " " << im.total() << endl;
                    uint16_t *ptr = im.ptr<uint16_t>(0);
                    size_t sz = im.total();
                    if (sz > max_image) {
                        shuffle(ptr, ptr +  sz, rng);
                        sz = max_image;
                    }
#pragma omp critical
                    {
                        insert(end(), ptr, ptr + sz);
                        ++progress;
                    }
                }
            }
            __gnu_parallel::sort(begin(), end());
        }
    };

    void CAlign::fit (vector<string> const &from_files,
                      vector<string> const &to_files) {
        Histogram from(from_files);
        Histogram to(to_files);
        unsigned last_from_v = 0;
        float last_to_v = 0;
        lookup.resize(from.back() + 1);
        lookup[last_from_v] = last_to_v;
        unsigned b = 0;
        for (;;) {
            if (b >= from.size()) break;
            unsigned e = b + 1;
            while (e < from.size() && from[b] == from[e]) ++e;
            unsigned from_v = from[b];
            unsigned to_index = (b + e - 1) * to.size() / (2 * from.size());
            if (to_index >= to.size()) to_index = to.size() - 1;
            float to_v = to[to_index];
            cout << from_v << " => "  << to_v << endl;
            if (from_v == 0) {
                last_from_v = from_v;
                last_to_v = to_v;
                lookup[last_from_v] = last_to_v;
            }
            else {
                for (unsigned x = last_from_v + 1; x <= from_v; ++x) {
                    //     y - last_to_v          x - last_from_v
                    //   ----------------- ==== --------------------
                    //   to_v - last_to_v       from_v - last_from_v
                    //
                    float y =  1.0 * (x - last_from_v) * (to_v - last_to_v) / (from_v - last_from_v)
                            + last_to_v;
                    lookup[x] = y;
                }
            }
            b = e;
        }
    }
}

using namespace std;
namespace fs = boost::filesystem;
extern char _binary_dicom_dic_start;
extern char _binary_dicom_dic_end;


void load_list (fs::path const &path, vector<string> *list) {
    fs::ifstream is(path);
    string line;
    list->clear();
    while (is >> line) {
        list->emplace_back(std::move(line));
    }
}

int main (int argc, char *argv[]) {
    fs::path from_list;
    fs::path to_list;
    fs::path model_path;
    {
        int verbose;
        namespace po = boost::program_options;
        po::options_description desc_visible("Allowed options");
        desc_visible.add_options()
            ("help,h", "produce help message.")
            ("from", po::value(&from_list), "")
            ("to", po::value(&to_list), "")
            ("model", po::value(&model_path), "")
            ;

        po::options_description desc("Allowed options");
        desc.add(desc_visible);

        po::positional_options_description p;
        p.add("from", 1);
        p.add("to", 1);
        p.add("model", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                         options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help") || from_list.empty() || to_list.empty() || model_path.empty()) {
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

    vector<string> from;
    vector<string> to;
    load_list(from_list, &from);
    load_list(to_list, &to);
    calign::CAlign align;
    align.fit(from, to);
    align.save(model_path.native());
    return 0;
}
