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
#include "image-io.h"



namespace calign {
    using namespace std;

    class Histogram: public vector<uint16_t> {
    public:
        // Sample pixels from a list of images, at most max_image pixels from
        // each image.
        // Then sort all the pixels.
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

void load_list (fs::path const &path, vector<string> *list) {
    fs::ifstream is(path);
    string line;
    list->clear();
    while (is >> line) {
        list->emplace_back(std::move(line));
    }
}

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
        p.add("from", 1);
        p.add("to", 1);
        p.add("model", 1);

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

    vector<string> from;
    vector<string> to;
    load_list(from_path, &from);
    load_list(to_path, &to);
    calign::CAlign align;
    align.fit(from, to);
    align.save(model_path.native());
    return 0;
}
