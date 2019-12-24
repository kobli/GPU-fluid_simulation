BIN=demo

build: $(BIN)

$(BIN): main.cpp utils.cpp
	g++ -g $^ -o $@ -lGL -lglut -lGLEW

test: $(BIN)
	primusrun ./$(BIN) 2>&1 | less

