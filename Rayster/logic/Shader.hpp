#pragma once

#include "common.h"

#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <fstream>

class Shader {
protected:
	Microsoft::WRL::ComPtr<ID3DBlob> binary;
	Microsoft::WRL::ComPtr<ID3DBlob> error_message;

	HRESULT compile(std::filesystem::path path, LPCSTR pTarget);

public:
	inline const LPVOID getBytecode() const { return binary->GetBufferPointer(); }
	inline const SIZE_T getBytecodeLength() const { return binary->GetBufferSize(); }
};

class VertexShader : public Shader {
public:
	VertexShader(std::filesystem::path path);
};

class PixelShader : public Shader {
public:
	PixelShader(std::filesystem::path path);
};