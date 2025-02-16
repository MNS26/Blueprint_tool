PKG_CONFIG ?= pkg-config
LIBS := -lprotobuf -lprotobuf-lite
CFLAGS := -DNDEBUG -g -Wall -Ilib/lz4/

# EXT := .exe
LIBS += `$(PKG_CONFIG) --libs protobuf`

all: blueprint-unpacker blueprint-repacker

.PHONY: clean
clean:
	rm blueprint-repacker.o blueprint-unpacker.o main-repacker.o main-unpacker.o smaz.o trailmakers.pb.o trailmakers.pb.h trailmakers.pb.cc blueprint-unpacker.exe blueprint-repacker.exe

blueprint-unpacker: main-unpacker.o blueprint-unpacker.o smaz.o trailmakers.pb.o
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

blueprint-repacker: main-repacker.o blueprint-repacker.o smaz.o trailmakers.pb.o
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

run: blueprint-unpacker
	./blueprint-unpacker -p Blueprint_202106251826128194.png -j out.json

#unpacker: blueprint-packer.o trailmakers.pb.o smaz.o lib/lz4/liblz4.a
#	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

smaz: smaz.cc
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

%.o: %.cc
	$(CXX) -c -o $@ $< $(CFLAGS)

#$(pkg-config protobuf --libs) -llz4 -lprotobuf-c -lprotoc

blueprint-packer.o: blueprint-packer.cc trailmakers.pb.h
blueprint-repacker.o: blueprint-repacker.cc trailmakers.pb.h
blueprint-unpacker.o: blueprint-unpacker.cc stb/stb_image.h smaz.h blueprint-unpacker.hpp trailmakers.pb.h
main-repacker.o: main-repacker.cc
main-unpacker.o: main-unpacker.cc smaz.h blueprint-unpacker.hpp trailmakers.pb.h
smaz.o: smaz.cc
trailmakers.pb.o: trailmakers.pb.cc

trailmakers.pb.cc trailmakers.pb.h: trailmakers.proto
	protoc trailmakers.proto --cpp_out=.

lib/lz4/liblz4.a:
	$(MAKE) -C lib/lz4 liblz4.a

install: blueprint-unpacker$(EXT)
	mkdir -pv $(out)/bin
	cp $^ $(out)/bin
