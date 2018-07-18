CFLAGS =	-Wall -std=c++11

LIBS =		-levent -levent_core

main: main.cpp Server.cpp
	g++ $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f main *~
