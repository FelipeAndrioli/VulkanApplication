#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>

#include "../../Utils/Helper.h"

#define MSVC

// TODO: make the compiler flag dynamic at compilation time

class Profiler {
public:
	enum UnitType {
		MS,
		US
	};

	struct Item {
		long long Duration;
		std::string FunctionId;
		UnitType UnitType;
	};

	std::unordered_map<std::string, std::vector<Item>> Items;

public:

	static Profiler* Get() {
		if (!m_Instance) {
			m_Instance = new Profiler();
		}

		return m_Instance;
	}

	void Add(const char* fileName, const char* functionId, long long duration, UnitType unitType) {
		if (Items.find(fileName) == Items.end()) {
			Items[fileName].push_back({ .Duration = duration, .FunctionId = functionId, .UnitType = unitType });
			return;
		}

		size_t FunctionIndex = 0;

		for (; FunctionIndex < Items[fileName].size(); ++FunctionIndex) {
			if (Items[fileName][FunctionIndex].FunctionId == functionId) {
				Items[fileName][FunctionIndex].Duration = duration;
				break;
			}
		}

		if (FunctionIndex == Items[fileName].size()) {
			Items[fileName].push_back({ .Duration = duration, .FunctionId = functionId, .UnitType = unitType });
		}
	}

	void Destroy() {
		Items.clear();

		delete m_Instance;
	}

private:
	Profiler() {};
	Profiler(Profiler& other)				= delete;
	Profiler(Profiler&& other)				= delete;
	void operator=(const Profiler& other)	= delete;
	void operator=(const Profiler&& other)	= delete;

	static Profiler* m_Instance;
};

class ScopedProfiler {
public:
	ScopedProfiler(const char* fileName, const char* functionId, Profiler::UnitType unitType) 
		:	
			Begin(std::chrono::high_resolution_clock::now()) {

#ifdef MSVC
		const char* ExtractedFileName = Helper::get_filename_msvc(fileName);
		strcpy(FileName, ExtractedFileName);
#else
		FileName(fileName);
#endif
		strcpy(FunctionId, functionId);

		UnitType = unitType;
	}

	~ScopedProfiler() {
		End = std::chrono::high_resolution_clock::now();

		long long duration = 0l;

		if (UnitType == Profiler::UnitType::MS) {
			duration = std::chrono::duration_cast<std::chrono::milliseconds>(End - Begin).count();
//			std::cout << FileName << " : " << FunctionId << " " << duration << "ms\n";
		}

		if (UnitType == Profiler::UnitType::US) {
			duration = std::chrono::duration_cast<std::chrono::microseconds>(End - Begin).count();
//			std::cout << FileName << " : " << FunctionId << " " << duration << "us\n";
		}

		Profiler::Get()->Add(FileName, FunctionId, duration, UnitType);
	}

public:

	char FileName[50];
	char FunctionId[50];

	std::chrono::high_resolution_clock::time_point Begin;
	std::chrono::high_resolution_clock::time_point End;

	Profiler::UnitType UnitType;
};

#define SCOPED_PROFILER_MS(name) ScopedProfiler profiler_ms_##__LINE__(__FILE__, name, Profiler::UnitType::MS);
#define SCOPED_PROFILER_US(name) ScopedProfiler profiler_us_##__LINE__(__FILE__, name, Profiler::UnitType::US);
