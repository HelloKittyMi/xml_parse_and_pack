CC=gcc
SOCFLAGS += -shared -fPIC

LDFLAGS =  -L.

OBJ1=util_xparse.o util_xadd.o
TARGET1=libxml.so
CFLAGS1 +=

OBJ2=test_parse_xml.o
TARGET2=test_parse_xml
CFLAGS2+=
LDFLAG2+= -lxml

TARGET= $(TARGET1) $(TARGET2)
.PHONY:all
all:$(TARGET)

$(TARGET1):$(OBJ1)
	$(CC)  $(SOCFLAGS) -o $(TARGET1) $(OBJ1)
$(TARGET2):$(OBJ2)
	$(CC)  $(LDFLAGS) $(LDFLAG2) -o $(TARGET2) $(OBJ2)


.PHONY:clean
clean:
	-rm -f *.o
    