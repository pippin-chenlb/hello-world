INCLUDE_DIRS := -I../common -I./include -I./gprs -I./cdma -I./tdr  -I../include -I../../tdros/toslib/include -I../../tdros/thirdparty/libuv-v1.9.1/include

CUR_SOURCE = ${wildcard *.c}
CUR_OBJS = ${patsubst %.c, %.o, $(CUR_SOURCE)}
DEPS := $(CUR_OBJS:%.o=%.d)

MAP_FILE = debug.map
GEN_MAP = -Wl,-Map,$(MAP_FILE)

#LIBS := -L../../toslib/ -ltos -L../../thirdparty/libs/ -luv
LIBS := -L../../tdros/toslib/ -ltos -L../../tdros/thirdparty/libuv-v1.9.1/.libs/ -luv

TARGET := debug

all:$(TARGET)

$(TARGET):$(CUR_OBJS)
	$(CC) -lpthread -lrt -rdynamic -ldl -lm $(GEN_MAP) $(LIBS) $^ -o $@

$(CUR_OBJS):%.o:%.c
	$(CC) -MMD $(CC_FLAGS) $(INCLUDE_DIRS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(CUR_OBJS) $(DEPS) $(TARGET) $(MAP_FILE)

