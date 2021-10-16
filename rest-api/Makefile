PROJECT=client
SOURCES=client.cpp requests.cpp helpers.cpp buffer.c
CC=g++
CFLAGS=-Wall -g
INCPATHS=nlohmann

INCFLAGS=$(foreach TMP,$(INCPATHS),-I$(TMP))

BINARY=$(PROJECT)

all: $(BINARY)

$(BINARY): $(SOURCES)
	$(CC) $(CFLAGS) $(INCFLAGS) $^ -o $@

run: $(BINARY)
	./$(BINARY)

clean:
	rm -f *.o $(BINARY)
