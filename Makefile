clean:
	rm -r bin
	rm -r build
prepare-mingw:
	cmake -S . -B build -G "MinGW Makefiles"
prepare-ninja:
	cmake -S . -B build -G "Ninja"
