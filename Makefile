PKG_CONFIG ?= pkg-config
LIBS := -lprotobuf -lprotobuf-lite 
CFLAGS := -DNDEBUG -g -Wall -Ilib/lz4/

ifeq ($(WINDOWS), 1)
  EXT := .exe
	LIBS += -lrpcrt4
else
  LIBS += `$(PKG_CONFIG) --libs protobuf` -luuid
endif

all: blueprint-unpacker blueprint-repacker blueprint-reimager

.PHONY: clean
clean:
	rm blueprint-repacker.o blueprint-unpacker.o main-repacker.o main-unpacker.o main-reimager.o smaz.o trailmakers.pb.o trailmakers.pb.h trailmakers.pb.cc blueprint-unpacker.exe blueprint-repacker.exe blueprint-imager.exe

blueprint-unpacker: main-unpacker.o blueprint-unpacker.o smaz.o trailmakers.pb.o
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

blueprint-repacker: main-repacker.o blueprint-repacker.o smaz.o trailmakers.pb.o
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

blueprint-reimager: main-reimager.o blueprint-unpacker.o smaz.o trailmakers.pb.o
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

smaz: smaz.cc
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

%.o: %.cc
	$(CXX) -c -o $@ $< $(CFLAGS)

#$(pkg-config protobuf --libs) -llz4 -lprotobuf-c -lprotoc

#blueprint-packer.o: blueprint-packer.cc trailmakers.pb.h
blueprint-repacker.o: blueprint-repacker.cc stb/stb_image.h stb/stb_image_write.h smaz.h blueprint-repacker.hpp trailmakers.pb.h
blueprint-unpacker.o: blueprint-unpacker.cc stb/stb_image.h smaz.h blueprint-unpacker.hpp trailmakers.pb.h
blueprint-reimager.o: blueprint-unpacker.cc stb/stb_image.h stb/stb_image_write.h smaz.h blueprint-unpacker.hpp trailmakers.pb.h

main-repacker.o: main-repacker.cc smaz.h blueprint-unpacker.hpp trailmakers.pb.h
main-unpacker.o: main-unpacker.cc smaz.h blueprint-unpacker.hpp trailmakers.pb.h
main-reimager.o: main-reimager.cc smaz.h blueprint-unpacker.hpp trailmakers.pb.h
smaz.o: smaz.cc
trailmakers.pb.o: trailmakers.pb.cc

trailmakers.pb.cc trailmakers.pb.h: trailmakers.proto
	protoc trailmakers.proto --cpp_out=.

lib/lz4/liblz4.a:
	$(MAKE) -C lib/lz4 liblz4.a

install: blueprint-unpacker$(EXT) blueprint-repacker$(EXT) blueprint-reimager$(EXT)
	mkdir -pv $(out)/bin
	cp $^ $(out)/bin
