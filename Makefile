DIR_INC = -I./include -I./../protocol/src -I./../plugin/unix/hiredis-0.14.0 -I./../mmo/include -I./../plugin/unix/libuv-v1.9.1/include -I./../plugin/unix/lua  -I./../plugin/unix/protobuf-3.3.0/src
DIR_SRC = ./src
DIR_OBJ = ./obj
DIR_LIB = ./lib

SRC = $(wildcard $(DIR_SRC)/*.cpp)
SRC_WITHOUT_DIR = $(notdir $(SRC))
OBJ = $(patsubst %.cpp, ${DIR_OBJ}/%.o, ${SRC_WITHOUT_DIR})
TARGET = $(DIR_LIB)/libredis.a

CC = g++
CFLAGS = -std=c++14 ${DIR_INC} -DNDEBUG -o3 -g -m64

all : PREPARE $(TARGET)

PREPARE : 
	echo $(SRC)
	echo $(OBJ)
	echo $(TARGET)
	mkdir -p obj
	mkdir -p lib

$(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@ `pkg-config --cflags --libs protobuf` -D_GLIBCXX_USE_CXX11_ABI=0

.PHONY:install
install :
	-cp ./lib/libredis.a /usr/local/lib 
	-cp ./lib/libredis.a ./../../../../bin/unix/
	
${TARGET}:$(OBJ)
	ar rcs ${TARGET} $(OBJ) 


.PHONY:clean
clean:
	-rm -rf lib
	-rm -rf obj
	rm -rf ${DIR_OBJ} $(DIR_LIB)


