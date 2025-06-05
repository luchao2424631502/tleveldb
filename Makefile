CC=g++
TARGET=tkv
CMAKE=cmake
INC_DIR=include/
LIB_DIR=
LIBS := -lfmt -lpmemobj -lpmem2
CXXFLAGS= -g
SRC= src/main.cc src/deviceManager.cc src/groupManager.cc

# test对象
TEST_INC_DIR= lib/leveldb/installed/include
TEST_SRC= test/test_main.cc src/deviceManager.cc src/groupManager.cc
TEST_LIBS= -lfmt -lpmemobj -lpmem2 -lgtest -lgtest_main 
TEST_LIB_DIR= lib/leveldb/installed/lib

.PHONY: all $(TARGET) THIRD LEVELDB clean test

all: $(TARGET)
	@echo "all"

test: LEVELDB
	$(CC) -I$(INC_DIR) $(CXXFLAGS) $(TEST_SRC) -o test/test_main $(TEST_LIBS)

$(TARGET): THIRD
	@echo "target"
	$(CC) -I$(INC_DIR) $(CXXFLAGS) $(SRC) -o $@ $(LIBS)

THIRD: LEVELDB 
	@echo "third"

LEVELDB:
	$(CMAKE) -B lib/leveldb/build -S lib/leveldb -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=lib/leveldb/installed
	$(CMAKE) --build lib/leveldb/build -j1024
	$(CMAKE) --install lib/leveldb/build

clean:
	rm -rf lib/leveldb/build/* $(TARGET)