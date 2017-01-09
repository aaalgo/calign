#ifndef AAALGO_CALIGN
#define AAALGO_CALIGN

#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <limits>
//#include <opencv2/opencv.hpp>

namespace calign {

    class CAlign {
        std::vector<float> lookup;

        uint16_t conv (uint16_t v) const {
            if (v >= lookup.size()) return lookup.back();
            return std::round(lookup[v]);
        }

        float conv (float v) const {
            if (v >= lookup.size() - 1) return lookup.back();
            if (v <= 0) return lookup.front();
            int l(v); // floor
            int r = l + 1;
            float lv = lookup[l];
            float rv = lookup[r];
            return lv * (r - v) + rv * (v - l);
            // case l = v, r = v + 1, should return lv
            // case r = v, l = r - 1, should return rv
        }
    public:
        void load (std::string const &path) {
            std::ifstream is(path.c_str(), std::ios::binary);
            if (!is) throw std::runtime_error("failed to open " + path);
            uint32_t v;
            is.read((char *)&v, sizeof(v));
            if (v > std::numeric_limits<uint16_t>::max()+1) {
                throw std::runtime_error("table too big");
            }
            lookup.resize(v);
            is.read((char *)&lookup[0], sizeof(lookup[0]) * lookup.size());
            if (!is) throw std::runtime_error("error reading " + path);
        }
        void save (std::string const &path) {
            std::ofstream os(path.c_str(), std::ios::binary);
            uint32_t v = lookup.size();
            os.write((char const *)&v, sizeof(v));
            os.write((char const *)&lookup[0], lookup.size() * sizeof(lookup[0]));
            if (!os) throw std::runtime_error("failed to write " + path);
        }
        void fit (std::vector<std::string> const &from,
                  std::vector<std::string> const &to);
        template <typename T>
        void apply (T *v, size_t n) {
            for (size_t i = 0; i < n; ++i) {
                v[i] = conv(v[i]);
            }
        }
    };
}

#endif
