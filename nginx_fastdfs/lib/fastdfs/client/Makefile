.SUFFIXES: .c .o .lo

COMPILE = $(CC) -Wall -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -g -O1 -DDEBUG_FLAG
ENABLE_STATIC_LIB = 0
ENABLE_SHARED_LIB = 1
INC_PATH = -I../common -I../tracker -I/usr/include/fastcommon
LIB_PATH =  -L/usr/lib64 -lpthread -lfastcommon
TARGET_PATH = /usr/bin
TARGET_LIB = /usr/lib64
TARGET_INC = /usr/include
CONFIG_PATH = /etc/fdfs

FDFS_STATIC_OBJS = ../common/fdfs_global.o ../common/fdfs_http_shared.o \
                   ../common/mime_file_parser.o ../tracker/tracker_proto.o \
                   ../tracker/fdfs_shared_func.o ../tracker/fdfs_server_id_func.o \
                   ../storage/trunk_mgr/trunk_shared.o \
                   tracker_client.o client_func.o \
                   client_global.o storage_client.o

STATIC_OBJS =  $(FDFS_STATIC_OBJS)

FDFS_SHARED_OBJS = ../common/fdfs_global.lo ../common/fdfs_http_shared.lo \
                   ../common/mime_file_parser.lo ../tracker/tracker_proto.lo \
                   ../tracker/fdfs_shared_func.lo ../tracker/fdfs_server_id_func.lo \
                   ../storage/trunk_mgr/trunk_shared.lo \
                   tracker_client.lo client_func.lo \
                   client_global.lo storage_client.lo

FDFS_HEADER_FILES = ../common/fdfs_define.h ../common/fdfs_global.h \
                    ../common/mime_file_parser.h ../common/fdfs_http_shared.h \
                    ../tracker/tracker_types.h ../tracker/tracker_proto.h \
                    ../tracker/fdfs_shared_func.h ../tracker/fdfs_server_id_func.h \
                    ../storage/trunk_mgr/trunk_shared.h \
                    tracker_client.h storage_client.h storage_client1.h \
                    client_func.h client_global.h fdfs_client.h

ALL_OBJS = $(STATIC_OBJS) $(FDFS_SHARED_OBJS)

ALL_PRGS = fdfs_monitor fdfs_test fdfs_test1 fdfs_crc32 fdfs_upload_file \
           fdfs_download_file fdfs_delete_file fdfs_file_info \
           fdfs_appender_test fdfs_appender_test1 fdfs_append_file \
           fdfs_upload_appender fdfs_regenerate_filename

STATIC_LIBS = libfdfsclient.a

SHARED_LIBS = libfdfsclient.so
CLIENT_SHARED_LIBS = libfdfsclient.so

ALL_LIBS = $(STATIC_LIBS) $(SHARED_LIBS)

all: $(ALL_OBJS) $(ALL_PRGS) $(ALL_LIBS)
libfdfsclient.so:
	$(COMPILE) -o $@ $< -shared $(FDFS_SHARED_OBJS) $(LIB_PATH)
libfdfsclient.a:
	ar rcs $@ $< $(FDFS_STATIC_OBJS)
.o:
	$(COMPILE) -o $@ $<  $(STATIC_OBJS) $(LIB_PATH) $(INC_PATH)
.c:
	$(COMPILE) -o $@ $<  $(STATIC_OBJS) $(LIB_PATH) $(INC_PATH)
.c.o:
	$(COMPILE) -c -o $@ $<  $(INC_PATH)
.c.lo:
	$(COMPILE) -c -fPIC -o $@ $<  $(INC_PATH)
install:
	mkdir -p $(TARGET_PATH)
	mkdir -p $(CONFIG_PATH)
	mkdir -p $(TARGET_LIB)
	mkdir -p /usr/lib
	cp -f $(ALL_PRGS) $(TARGET_PATH)
	if [ 0 -eq 1 ]; then cp -f $(STATIC_LIBS) $(TARGET_LIB); cp -f $(STATIC_LIBS) /usr/lib/;fi
	if [ 1 -eq 1 ]; then cp -f $(CLIENT_SHARED_LIBS) $(TARGET_LIB); cp -f $(CLIENT_SHARED_LIBS) /usr/lib/;fi

	mkdir -p $(TARGET_INC)/fastdfs
	cp -f $(FDFS_HEADER_FILES) $(TARGET_INC)/fastdfs
	if [ ! -f $(CONFIG_PATH)/client.conf.sample ]; then cp -f ../conf/client.conf $(CONFIG_PATH)/client.conf.sample; fi
clean:
	rm -f $(ALL_OBJS) $(ALL_PRGS) $(ALL_LIBS)

