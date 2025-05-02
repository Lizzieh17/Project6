# define libraries
ifeq ($(shell uname),Darwin)
LIB = -DMAC -framework OPENGL -framework GLUT
else
LIB = -lGL -lGLU -lglut
endif
LIB2 = libim/libim.a jpeg/libjpeg.a

# define makefile targets
CC = g++ -Wall -Wno-deprecated-declarations
BIN = ray_trace 

all: $(BIN)

clean: 
	/bin/rm -f $(BIN)

ray_trace: ray_trace.cpp ray_classes.cpp
	$(CC) -o ray_trace ray_trace.cpp ray_classes.cpp $(LIB)