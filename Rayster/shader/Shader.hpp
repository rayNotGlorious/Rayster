#pragma once

#include "common.h"

#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <fstream>

class Shader {
public:
	Shader(std::string_view name);
	~Shader();

	inline const void* getBuffer() const { return data; }
	inline size_t getSize() const { return size; }


private:
	void* data = nullptr;
	size_t size = 0;

};