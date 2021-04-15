all:
	$(CXX) -o builder builder.cc -std=c++17 -I.

regen: bin2c stage2/stage2.bin
	./bin2c stage2/stage2.bin stage2-bin.h stage2

.PHONY: regen
