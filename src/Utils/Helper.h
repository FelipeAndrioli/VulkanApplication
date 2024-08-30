#pragma once

#include <string>

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
}