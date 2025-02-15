PKG_CONFIG ?= pkg-config
#`$(PKG_CONFIG) --libs protobuf`
LIBS := -lprotobuf -Llib/lz4 -Llib/protobuf/build/ -Llib/protobuf/src/ -Ilib/protobuf/src 

all: blueprint-unpacker blueprint-repacker smaz.o trailmakers.pb.o lib/lz4/liblz4.a

blueprint-unpacker: main-unpacker.o blueprint-unpacker.o smaz.o trailmakers.pb.o lib/lz4/liblz4.a
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g 

blueprint-repacker: main-repacker.o blueprint-repacker.o smaz.o trailmakers.pb.o lib/lz4/liblz4.a
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g -static

run: blueprint-unpacker
	./blueprint-unpacker -p Blueprint_202106251826128194.png -j out.json

#unpacker: blueprint-packer.o trailmakers.pb.o smaz.o lib/lz4/liblz4.a
#	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

smaz: smaz.cc
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

%.o: %.cc
	$(CXX) -c -o $@ $< -g -Wall -Ilib/lz4/

#$(pkg-config protobuf --libs) -llz4 -lprotobuf-c -lprotoc

main-unpacker.o: main-unpacker.cc

main-repacker.o: main-repacker.cc

smaz.o: smaz.cc

blueprint-unpacker.o: blueprint-unpacker.cc trailmakers.pb.h

blueprint-repacker.o: blueprint-repacker.cc trailmakers.pb.h

trailmakers.pb.cc trailmakers.pb.h: trailmakers.proto
	lib/protobuf/build/protoc trailmakers.proto --cpp_out=.

lib/lz4/liblz4.a:
	$(MAKE) -C lib/lz4 liblz4.a

install: blueprint-unpacker
	mkdir -pv $(out)/bin
	cp $^ $(out)/bin