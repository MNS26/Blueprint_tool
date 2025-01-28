LIBS := `$(PKG_CONFIG) --libs protobuf`

blueprint-unpacker: blueprint-unpacker.o trailmakers.pb.o
	$(CXX) -o $@ $^ -llz4 $(LIBS) -g

%.o: %.cc
	$(CXX) -c -o $@ $< -g -Wall

#$(pkg-config protobuf --libs) -llz4 -lprotobuf-c -lprotoc

blueprint-unpacker.o: blueprint-unpacker.cc trailmakers.pb.h

trailmakers.pb.cc trailmakers.pb.h: trailmakers.proto
	protoc trailmakers.proto --cpp_out=.

install: blueprint-unpacker
	mkdir -pv $(out)/bin
	cp $^ $(out)/bin

#blueprint-unpacker: blueprint-unpacker.cc stb/stb_image.h stb/stb_image_write.h lz4/lz4.h  lz4/lz4frame.h lz4/lz4frame_static.h lz4/lz4hc.h lz4/xxhash.h trailmakers.pb.h
#	$(CXX) blueprint-unpacker.cc lz4/lz4.c lz4/lz4frame.c lz4/lz4hc.c lz4/xxhash.c trailmakers.pb.cc -lprotobuf -o "Blueprint Unpacker"

# trailmakers.pb.cc
# trailmakers.pd.cc
# -static-libgcc -static-libstdc++         -Bstatic -g -lm 
# -static-libgcc -static-libstdc++ -static -Bstatic -g -lm 
