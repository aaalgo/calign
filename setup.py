from distutils.core import setup, Extension

picpac = Extension('_calign',
        language = 'c++',
        extra_compile_args = ['-O3', '-std=c++1y'], 
        include_dirs = ['/usr/local/include'],
        libraries = ['opencv_highgui', 'opencv_core', 'boost_filesystem', 'boost_system', 'boost_python'],
        library_dirs = ['/usr/local/lib'],
        sources = ['python-api.cpp'],
        depends = ['calign.h'])

setup (name = 'calign',
       version = '0.0.1',
       url = 'https://github.com/aaalgo/calign',
       author = 'Wei Dong',
       author_email = 'wdong@wdong.org',
       license = 'BSD',
       description = 'This is a demo package',
       ext_modules = [calign],
       py_modules = ['picpac.mxnet', 'picpac.neon'],
       )
