# 编译器选择
CXX := g++

# 编译选项
CXXFLAGS := -Wall -Wextra -std=c++11 -O2

# 链接器选项
LDFLAGS := 

# 最终要生成的两个可执行文件
TARGETS := client tcpepoll

# client 相关的源文件和目标文件
CLIENT_SRCS := client.cpp
CLIENT_OBJS := $(CLIENT_SRCS:.cpp=.o)

# tcpepoll 相关的源文件和目标文件
TCPEPOLL_SRCS := tcpepoll.cpp
TCPEPOLL_OBJS := $(TCPEPOLL_SRCS:.cpp=.o)

# 公共头文件
HEADERS := net.hpp

# 默认目标：编译生成所有的可执行文件
all: $(TARGETS)

# 生成 client 可执行文件
client: $(CLIENT_OBJS)
	$(CXX) $(CLIENT_OBJS) -o $@ $(LDFLAGS)

# 生成 tcpepoll 可执行文件
tcpepoll: $(TCPEPOLL_OBJS)
	$(CXX) $(TCPEPOLL_OBJS) -o $@ $(LDFLAGS)

# 编译规则：将 .cpp 文件编译成 .o 文件
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理目标
clean:
	rm -f $(TARGETS) *.o

# 声明伪目标
.PHONY: all clean
