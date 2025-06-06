TARGET=tkv
CC=g++
CMAKE=cmake
AR=ar
CXXFLAGS= -Wall
DEBUGCXXFLAGS=-g
RELEASECXXFLAGS=-O2
DEPFLAGS= -MMD -MP $(CXXFLAGS)

INC_DIR=include/ lib/leveldb/include/leveldb
INC_DIR:=$(foreach i,$(INC_DIR),$(addprefix -I, $i))
LIBS := -lfmt -lpmemobj -lpmem2
SRC_DIR=src
LIB_DIR=lib
DEP_DIR=.deps
OBJ_DIR=.deps
# target 对象
# COMMON 库对象
COMMON_LIB=$(LIB_DIR)/libcommon.a
COMMON_LIB_SRC_FILES=$(filter-out $(SRC_DIR)/main%,$(wildcard $(SRC_DIR)/*.cc))
COMMON_LIB_OBJ_FILES=$(patsubst $(SRC_DIR)/%.cc, $(OBJ_DIR)/%.o, $(COMMON_LIB_SRC_FILES))
COMMON_LIB_FLAGS=-Llib/ -lcommon
# TEST 对象
TEST=testmain
TEST_DIR=test
TEST_SRC_FILE=$(TEST_DIR)/test_main.cc
TEST_OBJ_FILE=$(patsubst $(TEST_DIR)/%.cc, $(OBJ_DIR)/%.o, $(TEST_SRC_FILE))
TEST_LIBS= -lgtest -lgtest_main 
TEST_LIBS+=$(LIBS)

# 中间使用变量
ALL_SRC_FILES=$(wildcard $(SRC_DIR)/*.cc)
TARGET_OBJ_FILE=$(OBJ_DIR)/main.o
ALL_OBJ_FILES=$(patsubst $(SRC_DIR)/%.cc, $(OBJ_DIR)/%.o, $(ALL_SRC_FILES))
DEP_FILES=$( $(SRC_DIR)/%.cc, $(DEP_DIR)/%.d, $(ALL_SRC_FILES))

.PHONY: all debug release $(TARGET) $(TEST) $(COMMON_LIB) THIRD LEVELDB clean dirs

all: debug
	@echo "make all = make debug"

debug: CXXFLAGS += $(DEBUGCXXFLAGS)
debug: dirs $(TARGET) $(TEST) $(COMMON_LIB)
release: CXXFLAGS += $(RELEASECXXFLAGS)
release: dirs $(TARGET) $(TEST) $(COMMON_LIB)
dirs:
	@mkdir -p $(DEP_DIR) $(OBJ_DIR)

$(TARGET): $(TARGET_OBJ_FILE) $(COMMON_LIB) # THIRD 
	$(CC) $(INC_DIR) $(CXXFLAGS) $(TARGET_OBJ_FILE) -o $@ $(COMMON_LIB_FLAGS) $(LIBS)
	rm -rf $(ALL_OBJ_FILES)

$(TEST): $(TEST_OBJ_FILE) $(COMMON_LIB)
	$(CC) $(INC_DIR) $(CXXFLAGS) $(TEST_OBJ_FILE) -o $@ $(COMMON_LIB_FLAGS) $(TEST_LIBS)

$(COMMON_LIB): $(COMMON_LIB_OBJ_FILES)
	$(AR) rcs $@ $^

# 编译所有文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc $(ALL_SRC_FILES)
	$(CC) $(INC_DIR) $(DEPFLAGS) -c $< -o $@ $(LIBS)

$(TEST_OBJ_FILE): $(TEST_SRC_FILE)
	$(CC) $(INC_DIR) $(DEPFLAGS) -c $^ -o $@ $(TEST_LIBS)

THIRD: LEVELDB 
	@echo "third"

LEVELDB:
	$(CMAKE) -B lib/leveldb/build -S lib/leveldb -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=lib/leveldb/installed
	$(CMAKE) --build lib/leveldb/build -j1024
	$(CMAKE) --install lib/leveldb/build

-include $(DEP_FILES)

tmp:
	@echo ${COMMON_LIB_SRC_FILES}
	@echo ${COMMON_LIB_OBJ_FILES}
	@echo ${ALL_OBJ_FILES}
	@echo ${ALL_SRC_FILES}
	@echo ${INC_DIR}
	@echo ${TEST_SRC_FILE}
	@echo ${TEST_OBJ_FILE}

clean:
	rm -rf $(TARGET) $(TEST) $(ALL_OBJ_FILES) $(TEST_OBJ_FILE) $(COMMON_LIB) $(DEP_FILES)