PKG_CONFIG ?= pkg-config
LIBS := `$(PKG_CONFIG) --libs protobuf` -L./lib/lz4 


blueprint-unpacker: main-unpacker.o blueprint-packer.o smaz.o trailmakers.pb.o lib/lz4/liblz4.a
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

blueprint-unpacker-win: main-unpacker.o blueprint-packer.o smaz.o trailmakers.pb.o lib/lz4/liblz4.a
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

run: blueprint-unpacker
	./blueprint-unpacker -p Blueprint_202106251826128194.png -j out.json

blueprint-packer: blueprint-packer.o trailmakers.pb.o smaz.o lib/lz4/liblz4.a
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

smaz: smaz.cc
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

%.o: %.cc
	$(CXX) -c -o $@ $< -g -Wall -Ilib/lz4/

#$(pkg-config protobuf --libs) -llz4 -lprotobuf-c -lprotoc

blueprint-unpacker.o: main-unpacker.cc

smaz.o: smaz.cc

blueprint-packer.o: blueprint-packer.cc trailmakers.pb.h

trailmakers.pb.cc trailmakers.pb.h: trailmakers.proto
	protoc trailmakers.proto --cpp_out=.

lib/lz4/liblz4.a:
	$(MAKE) -C lib/lz4 liblz4.a

install: blueprint-unpacker
	mkdir -pv $(out)/bin
	cp $^ $(out)/bin