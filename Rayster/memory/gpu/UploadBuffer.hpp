#pragma once

#include <initializer_list>
#include <vector>

#include "common.h"
#include "display/Display.hpp"
#include "memory/Model.hpp"

class UploadBuffer {
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	std::vector<Model> assets;
	SIZE_T size;

public:
	UploadBuffer(std::initializer_list<Model> list);

	inline ID3D12Resource* getBuffer() {
		return buffer.Get();
	}

	inline SIZE_T getSize() const {
		return size;
	}
};