LIB_OBJS = scene.o visuals.o sdl_utils.o convo.o item.o action.o time.o game.o inventory.o type.o character.o

CC = g++
COMPILER_FLAGS = -w -g
LINKER_FLAGS = -lSDL2 -lSDL2_ttf  -lSDL2_image -lSDL2_mixer
LIB_DIR = /usr/local/lib/trail
LIB_HEADER_DIR = /usr/local/include/trail
LIB_NAME = libtrail.a

all: $(LIB_OBJS)
	ar rcs $(LIB_NAME) $(LIB_OBJS)
game.o: 	game.cpp
	$(CC) -c game.cpp  $(COMPILER_FLAGS)
scene.o :	scene.cpp scene.hpp
	$(CC) -c scene.cpp  $(COMPILER_FLAGS)
action.o:	action.cpp
	$(CC) -c action.cpp  $(COMPILER_FLAGS)
character.o:	character.cpp character.hpp
	$(CC) -c character.cpp  $(COMPILER_FLAGS)
inventory.o:	inventory.cpp
	$(CC) -c inventory.cpp  $(COMPILER_FLAGS)
item.o:	item.cpp item.hpp
	$(CC) -c item.cpp  $(COMPILER_FLAGS)
convo.o :	convo.cpp convo.hpp
	$(CC) -c convo.cpp  $(COMPILER_FLAGS)
visuals.o :	visuals.cpp visuals.hpp
	$(CC) -c visuals.cpp  $(COMPILER_FLAGS)
sdl_utils.o: sdl_utils.cpp
	$(CC) -c sdl_utils.cpp $(COMPILER_FLAGS)
time.o:	time.cpp
	$(CC) -c time.cpp $(COMPILER_FLAGS)
type.o:	type.cpp
	$(CC) -c type.cpp $(COMPILER_FLAGS)
clean:
	rm *.o -fr libtrail.a

dep_build_and_install_dump_lib:
	git clone https://github.com/dedobbin/dump_libpp.git
	$(MAKE) -C dump_libpp install
	sudo rm -fr dump_libpp

dep_install_sdl_dev:
	#TODO: other pckmanagers
	apt-get install libsdl2-dev
	apt-get install libsdl2-image-dev
	apt-get install libsdl2-ttf-dev