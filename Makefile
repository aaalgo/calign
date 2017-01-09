.PHONY:	all clean release check
CXX = g++
CC = g++
OPENMP =-fopenmp
VERSION=$(shell git describe --always)
CXXFLAGS += -D_GLIBCXX_PARALLEL $(OPENMP) -g -O3 -std=gnu++11 -I/opt/cbox/include
LDFLAGS += $(OPENMP) -static -L/opt/cbox/lib
LDLIBS = \
         -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_core \
	 -lturbojpeg -ltiff -lpng -lwebp -llibjasper -lippicv \
         -ldcmimgle -ldcmdata -loflog -lofstd \
         -lboost_timer -lboost_chrono -lboost_program_options -lboost_thread -lboost_filesystem -lboost_system \
	 -lglog -lgflags -lunwind \
         -lz -lrt -ldl -lpthread 

PROGS = calign-fit calign-apply

all:	check $(PROGS)

calign-fit:	calign-fit.o dicom.o

calign-apply:	calign-apply.o dicom.o

dicom.o:	dicom.dic
	objcopy -I binary -B i386:x86-64 -O elf64-x86-64 $^ $@

check:
	@if [ ! -d /opt/cbox ]; then echo run ./make.sh to build ; false ; fi

clean:
	rm *.o $(PROGS)
