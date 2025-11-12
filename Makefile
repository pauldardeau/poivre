# Copyright Paul Dardeau, SwampBits LLC 2017
# BSD License

CC = c++
CC_OPTS = -c -std=c++20 -shared -fPIC -pthread

LIB_NAME = poivre.so

OBJS = TestCase.o \
TestSuite.o \

all : $(LIB_NAME)

clean :
	rm -f *.o
	rm -f $(LIB_NAME)
	
$(LIB_NAME) : $(OBJS)
	$(CC) -shared -fPIC -pthread $(OBJS) -o $(LIB_NAME)

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
