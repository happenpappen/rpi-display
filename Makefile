#
# This Makefile is based on the library examples.
#
SCP=/usr/bin/scp
SSH=/usr/bin/ssh
SSH_TARGET=pi@display
SCP_TARGET="$(SSH_TARGET):"
SRC_DIR=/home/cs/DevelopmentProjects/rpi-display

CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter -Wno-deprecated-declarations -I$(RGB_INCDIR)
CXXFLAGS=$(CFLAGS)
OBJECTS=rpi-display.o abeliansandpile.o gameoflife.o imagescroller.o langtonsant.o pong.o tetrisclock.o utility.o
BINARIES=rpi-display

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
RGB_LIB_DISTRIBUTION=rpi-rgb-led-matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lmosquittopp

all : $(RGB_LIBRARY) $(BINARIES)

$(RGB_LIBRARY): FORCE
		HARDWARE_DESC="adafruit-hat" $(MAKE) -C $(RGB_LIBDIR)

rpi-display: $(OBJECTS)
	$(CXX) -o rpi-display $(OBJECTS) $(LDFLAGS)

# All the binaries that have the same name as the object file.q
% : %.o $(RGB_LIBRARY)
	$(CXX) $< -o $@ $(LDFLAGS)

%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES)
	HARDWARE_DESC="adafruit-hat" $(MAKE) -C $(RGB_LIBDIR) clean

deploy:
	$(SSH) $(SSH_TARGET) "rm -rf rpi-display"
	$(SCP) -qr $(SRC_DIR) $(SCP_TARGET)
	$(SSH) $(SSH_TARGET) "cd rpi-display;make clean;make all"

FORCE:
.PHONY: FORCE
