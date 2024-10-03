clean:
	rm -r bin
	rm -r build
prepare-mingw:
	cmake -S . -B build -G "MinGW Makefiles" -DRUNTIME_SHADER_COMPILE=false
prepare-ninja:
	cmake -S . -B build -G "Ninja" -DRUNTIME_SHADER_COMPILE=false
