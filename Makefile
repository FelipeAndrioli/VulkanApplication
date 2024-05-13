clean:
	rm -r bin
	rm -r build
prepare:
	cmake -S . -B build -G "MinGW Makefiles"
