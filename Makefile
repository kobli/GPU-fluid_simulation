BIN=demo

build: $(BIN)

$(BIN): *.cpp
	g++ -O2 $^ -o $@ -lGL -lglut -lGLEW

doc: *.hpp
	doxygen Doxyfile

pack-src: *.hpp *.cpp Makefile shaders/*
	tar -cvzf src.tgz $^

pack-bin: ${BIN} shaders/
	tar -cvzf linux.tgz $^
