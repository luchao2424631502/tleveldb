CC=g++
TARGET=tkv
CMAKE=cmake
INC_DIR=include/
LIB_DIR=
LIBS := -lfmt -lpmemobj -lpmem2
CXXFLAGS= -g

.PHONY: all $(TARGET) THIRD LEVELDB clean

all: $(TARGET)
	@echo "all"

$(TARGET): THIRD
	@echo "target"
	$(CC) -I$(INC_DIR) $(CXXFLAGS) src/main.cc -o $@ $(LIBS)

THIRD: LEVELDB 
	@echo "third"

LEVELDB:
	$(CMAKE) -B lib/leveldb/build -S lib/leveldb -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=lib/leveldb/installed
	$(CMAKE) --build lib/leveldb/build -j1024
	$(CMAKE) --install lib/leveldb/build

clean:
	rm -rf lib/leveldb/build/* $(TARGET)