#pragma once

#include <string>
#include <fstream>

namespace Helper {

	template<class T>
	constexpr void hash_combine(std::size_t& seed, const T& v) {
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	static std::string get_directory(const std::string& path) {
		std::string temp = path;

		int lastSlashIndex = 0;
		int fileNameSize = 0;

		for (size_t i = 0; i < temp.size(); i++) {
			if (path[i] == '/') {
				lastSlashIndex = i + 1;
				fileNameSize = 0;
			}

			if (lastSlashIndex > 0)
				fileNameSize++;
		}

		return temp.erase(lastSlashIndex, fileNameSize).c_str();
	}

	static std::string get_directory_name(const std::string& path) {
		
		std::string temp = path;

		int lastSlashIndex = 0;
		int beforeLastSlashIndex = 0;
		int directoryNameSize = 0; 

		for (size_t i = temp.size() - 1; i > 0; i--) {
			if (path[i] == '/' && lastSlashIndex == 0) {
				lastSlashIndex = i;
			} else if (path[i] == '/' && lastSlashIndex != 0 && beforeLastSlashIndex == 0) {
				beforeLastSlashIndex = i + 1;
			}

			if (lastSlashIndex > 0)
				directoryNameSize++;
		}

		temp = temp.erase(lastSlashIndex, temp.size() - 1).c_str();
		temp = temp.erase(0, beforeLastSlashIndex).c_str();

		return temp;
	}

	static std::string get_filename(const std::string& path) {
		std::string temp = path;

		int lastSlashIndex = 0;
		int fileNameSize = 0;

		for (size_t i = 0; i < temp.size(); i++) {
			if (path[i] == '/') {
				lastSlashIndex = i + 1;
				fileNameSize = 0;
			}

			if (lastSlashIndex > 0)
				fileNameSize++;
		}


		return temp.erase(0, lastSlashIndex).c_str();
	}

	static const char* get_filename_msvc(const char* file_path) {
		char FileName[50] = {};
		
		size_t FilePathSize = strlen(file_path);

		int LastSlashIndex = -1;

		for (size_t i = FilePathSize; i > 0; --i) {
			if (file_path[i] == '\\') {
				LastSlashIndex = i;
				break;
			}
		}

		if (LastSlashIndex == -1)
			return file_path;

		size_t FileNameCharIndex = 0;

		for (size_t i = LastSlashIndex + 1; i < FilePathSize; i++) {
			FileName[FileNameCharIndex++] = file_path[i];
		}

		FileName[FileNameCharIndex] = '\0';

		return FileName;
	}

	bool inline file_exists(const std::string& path) {
		std::ifstream f(path.c_str());
		return f.good();
	}
}
