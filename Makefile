BIN=demo

build: $(BIN)

$(BIN): *.cpp
	g++ -O2 $^ -o $@ -lGL -lglut -lGLEW

test: $(BIN)
	primusrun ./$(BIN) 2>&1 | less

