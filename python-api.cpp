#include <fstream>
#include <boost/ref.hpp>
#include <boost/python.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/raw_function.hpp>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarrayobject.h>
#include "picpac.h"
#include "picpac-cv.h"
using namespace boost::python;
using namespace picpac;

namespace {

template <typename T>
T *get_ndarray_data (object &o) {
    PyArrayObject *nd = reinterpret_cast<PyArrayObject *>(o.ptr());
    return reinterpret_cast<T*>(PyArray_DATA(nd));
}


}

BOOST_PYTHON_MODULE(_picpac)
{
    scope().attr("__doc__") = "CAlign Python API";
    /*
    class_<NumpyBatchImageStream, boost::noncopyable>("ImageStream", no_init)
        .def("__init__", raw_function(create_image_stream), "exposed ctor")
        .def("__iter__", raw_function(return_iterator))
        .def(init<string, NumpyBatchImageStream::Config const&>()) // C++ constructor not exposed
        .def("next", &NumpyBatchImageStream::next)
        .def("size", &NumpyBatchImageStream::size)
        .def("reset", &NumpyBatchImageStream::reset)
        .def("categories", &NumpyBatchImageStream::categories)
    ;
    class_<Writer>("Writer", init<string>())
        .def("append", append1)
        .def("append", append2)
    ;
    def("encode_raw", ::encode_raw_ndarray);
    def("write_raw", ::write_raw_ndarray);
    */
}

