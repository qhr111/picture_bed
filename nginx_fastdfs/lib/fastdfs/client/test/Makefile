.SUFFIXES: .c .o

COMPILE = $(CC) -Wall -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -g -O1 -DDEBUG_FLAG
INC_PATH = -I/usr/include/fastcommon -I/usr/include/fastdfs \
           -I/usr/local/include/fastcommon -I/usr/local/include/fastdfs
LIB_PATH = -L/usr/local/lib -lfastcommon -lfdfsclient  -L/usr/lib64 -lpthread
TARGET_PATH = $(TARGET_PATH)

ALL_OBJS = 

ALL_PRGS = fdfs_monitor fdfs_test fdfs_test1

all: $(ALL_OBJS) $(ALL_PRGS)
.o:
	$(COMPILE) -o $@ $<  $(SHARED_OBJS) $(LIB_PATH) $(INC_PATH)
.c:
	$(COMPILE) -o $@ $<  $(ALL_OBJS) $(LIB_PATH) $(INC_PATH)
.c.o:
	$(COMPILE) -c -o $@ $<  $(INC_PATH)
install:
	cp -f $(ALL_PRGS) $(TARGET_PATH)
clean:
	rm -f $(ALL_OBJS) $(ALL_PRGS)

