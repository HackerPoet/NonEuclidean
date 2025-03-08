CXX=c++ -c
CXXFLAGS=-O3 -std=c++11
LINK=c++
LINKFLAGS=-lm -lSDL2 -lSDL2main -lGLEW 

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
LINKFLAGS += -lGL
endif

ifeq ($(UNAME), Darwin)
LINKFLAGS += -framework OpenGL 
endif

EXE=NonEuclidean

HEADERS=Camera.h Collider.h Engine.h Floorplan.h FrameBuffer.h GameHeader.h \
	Ground.h House.h Input.h Mesh.h Object.h Physical.h Pillar.h PillarRoom.h \
	Player.h Portal.h Resources.h Scene.h Shader.h Sky.h Sphere.h Statue.h \
	Texture.h Timer.h Tunnel.h Vector.h \
	Level1.h Level2.h Level3.h Level4.h Level5.h Level6.h

OBJS=Camera.o Collider.o Engine.o Engine_Win32.o Engine_SDL2.o \
	FrameBuffer.o Input.o \
	Level1.o Level2.o Level3.o Level4.o Level5.o Level6.o \
	Main.o Mesh.o Object.o Physical.o Player.o Portal.o \
	Resources.o Shader.o Texture.o

.PHONY: all clean

all: ${EXE}

clean:
	rm ${OBJS}

${EXE}: ${OBJS}
	${LINK} $^ ${LINKFLAGS} -o ${EXE}

%.o : %.cpp ${HEADERS}
	${CXX} ${CXXFLAGS} $*.cpp -o $*.o
