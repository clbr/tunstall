CXXFLAGS += -Wall -Wextra -g
LDFLAGS += -Wl,-O1

NAME = tunstall
SRC = $(wildcard *.cpp *.c)

.PHONY: all clean

all: $(NAME)

$(NAME): $(SRC)
	$(CXX) $(SRC) -o $(NAME) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f *.o $(NAME)
