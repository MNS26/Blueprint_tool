PKG_CONFIG ?= pkg-config
LIBS := `$(PKG_CONFIG) --libs protobuf`

blueprint-unpacker: blueprint-unpacker.o trailmakers.pb.o
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

blueprint-repacker: blueprint-repacker.o trailmakers.pb.o
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

smaz: smaz.c
	$(CXX) -o $@ $^ 

run: blueprint-unpacker
	./blueprint-unpacker -p Blueprint_202106251826128194.png -j out.json
run-legacy:
	./blueprint-unpacker -p /home/noah/Documents/TrailMakers/Blueprints/Blueprint_202009222043333752.png -j out.json

%.o: %.cc
	$(CXX) -c -o $@ $< -g -Wall

#$(pkg-config protobuf --libs) -llz4 -lprotobuf-c -lprotoc

blueprint-unpacker.o: blueprint-unpacker.cc trailmakers.pb.h

blueprint-repacker.o: blueprint-repacker.cc trailmakers.pb.h

trailmakers.pb.cc trailmakers.pb.h: trailmakers.proto
	protoc trailmakers.proto --cpp_out=.

install: blueprint-unpacker
	mkdir -pv $(out)/bin
	cp $^ $(out)/bin