CXXFLAGS =	-O0 -g -Wall -fmessage-length=0 `mysql_config --cflags --libs` -MMD -MP -Wall

OBJS =		RRMode.o master_thread.o worker_threads.o init_configure.o printertomysql.o ssl.o

LIBS =

TARGET =        3d_server	

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) `mysql_config --cflags --libs` -lcrypto -lssl -lpthread -llog4cxx -lboost_system -lboost_filesystem -levent -Wl,-rpath,/usr/local/lib

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
