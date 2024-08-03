# 同级目录
CUR_DIR = .

# 头文件还包含
INCLUDE = -I$(CUR_DIR)

# 库文件
LIBS_PATH =
LIBS =

# 获取当前目录下的所有cpp文件
SRC = $(wildcard $(CUR_DIR)/*.cpp)

# 将src中的所有.cpp文件替换为.o文件
OBJS = $(patsubst %.cpp,%.o,$(SRC)) 

# 库名
EXEC = libeasymp3encoder.so

cc = g++

FLAGS = -fPIC

all: $(OBJS) $(EXEC)

$(OBJS): $(SRC)
	$(cc) $(FLAGS) -c $(SRC) $(INCLUDE) $(LIBS_PATH) $(LIBS)

$(EXEC): $(OBJS)
	$(cc) $(FLAGS) -shared -o $(EXEC) $(OBJS)
	rm -f *.o

.PHONY: clean
clean:
	rm -f *.o *.so
