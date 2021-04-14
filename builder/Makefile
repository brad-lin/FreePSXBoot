all:
	$(CXX) -o builder builder.cc -std=c++17 -I.

regen:
	$(MAKE) -C stage2
	$(CC) -o bin2c bin2c.c
	./bin2c stage2/stage2.bin stage2-bin.h stage2
