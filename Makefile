BIN=demo

build: $(BIN)

$(BIN): *.cpp
	g++ -O2 $^ -o $@ -lGL -lglut -lGLEW

doc:
	doxygen Doxyfile

