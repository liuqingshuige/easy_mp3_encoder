# 获取当前目录下的所有c文件
SRC = $(wildcard *.cpp) 
# 将src中的所有.c文件替换为.o文件
OBJS = $(patsubst %.cpp,%.o,$(SRC))
# 编译器
cc = arm-linux-g++
# 头文件包含路径
INCLUDE = -I./lib/
# 库文件
LIBS = -leasymp3encoder
# 库文件
LIBS_PATH = -L./lib/
# 目标执行文件
TARGET = mp3encode

$(TARGET): $(OBJS)
	$(cc) -o $(TARGET) $(OBJS) $(LIBS_PATH) $(LIBS)
	rm -f $(OBJS)
	
$(OBJS): $(SRC)	
	$(cc) -c -w $(SRC) $(INCLUDE) $(LIBS_PATH) $(LIBS)

.PHONY: clean
clean:
	rm -f *.o $(TARGET)


