#include "Shader.hpp"
#include <d3dcompiler.h>
#include <sstream>

HRESULT Shader::compile(std::filesystem::path path, LPCSTR pTarget) {
	UINT flags1 = D3DCOMPILE_ENABLE_STRICTNESS
		| D3DCOMPILE_WARNINGS_ARE_ERRORS
		| D3DCOMPILE_ALL_RESOURCES_BOUND;

#ifdef _Debug
	flags1 |= D3DCOMPILE_DEBUG;
#endif

	return D3DCompileFromFile(
		path.wstring().data(),
		nullptr,
		nullptr,
		"main",
		pTarget,
		flags1,
		0,
		&binary,
		&error_message
	);
}

VertexShader::VertexShader(std::filesystem::path path) {
	HRESULT result = compile(path, "vs_5_1");

	if (result != S_OK) {
		std::wostringstream error;
		error << L"There was an error compiling vertex shader, '" << path.wstring() << L"'\n";

		if (error_message->GetBufferPointer() != nullptr) {
			error << error_message->GetBufferPointer();
		} else {
			error << L".";
		}

		OutputDebugStringW(error.str().data());
	}
}

PixelShader::PixelShader(std::filesystem::path path) {
	HRESULT result = compile(path, "ps_5_1");

	if (result != S_OK) {
		std::wostringstream error;
		error << L"[ERROR] There was an error compiling pixel shader, '" << path.wstring() << L"'\n";

		if (error_message.Get() != nullptr) {
			error << static_cast<char*>(error_message->GetBufferPointer()) << "\n";
		} else {
			error << L"The error couldn't be resolved\n";
		}

		OutputDebugStringW(error.str().data());
	}
}