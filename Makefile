TARGET = bin/itlview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: clean default
	./$(TARGET) -f "./resources/iTunes 10.2.2 Library with single track.itl" -p
	#./$(TARGET) -f ./iTunes_Library.itl -p

default: $(TARGET)

clean:
	rm -f obj/*.o
	rm -f bin/*

$(TARGET): $(OBJ)
	gcc -o $@ $? -lcrypto -lz

# Uncomment to compile without debug info
#obj/%.o : src/%.c
#  gcc -c "$(<)" -o "$(@)" -Iinclude; \

# To add debug flag
obj/%.o : src/%.c	
	gcc -c "$(<)" -o "$(@)" -Iinclude -g
