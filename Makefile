all:
	g++ -O3 -Isrc/include -Lsrc/lib -o main src/main.cpp src/tinyfiledialogs.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lcomdlg32 -lole32 -luuid -lshlwapi