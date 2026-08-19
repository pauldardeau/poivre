# Copyright Paul Dardeau, SwampBits LLC 2017
# BSD License

CC = c++
CC_OPTS = -c -std=c++20 -fPIC -pthread -Wall -Wextra

LIB_NAME = poivre.so
TEST_BIN = poivre_tests

OBJS = TestCase.o \
TestSuite.o \

all : $(LIB_NAME)

test : $(TEST_BIN)
	./$(TEST_BIN)

clean :
	rm -f *.o
	rm -f $(LIB_NAME)
	rm -f $(TEST_BIN)

$(LIB_NAME) : $(OBJS)
	$(CC) -shared -fPIC -pthread $(OBJS) -o $(LIB_NAME)

$(TEST_BIN) : TestCase.cpp TestSuite.cpp tests/SelfTests.cpp
	$(CC) -std=c++20 -I. -pthread -Wall -Wextra $^ -o $(TEST_BIN)

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
