CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

TARGET = server
OBJS = 	buffer/*.cpp log/*.cpp pool/*.cpp timer/*.cpp		\
		http/*.cpp epoller/*.cpp	\
		webserver.cpp main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o bin/${TARGET}
	-pthread -lmysqlclient

clean:
	rm -rf bin/$(TARGET) 