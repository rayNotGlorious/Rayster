#include "Shader.hpp"

Shader::Shader(std::string_view name) {
	static std::filesystem::path shaderDirectory;

	if (shaderDirectory.empty()) {
		wchar_t moduleFileName[MAX_PATH];
		GetModuleFileName(nullptr, moduleFileName, MAX_PATH);

		shaderDirectory = moduleFileName;
		shaderDirectory.remove_filename();
	}

	std::ifstream shaderIn(shaderDirectory / name, std::ios::binary);
	if (shaderIn.is_open()) {
		shaderIn.seekg(0, std::ios::end);
		size = shaderIn.tellg();
		shaderIn.seekg(0, std::ios::beg);
		data = malloc(size);

		if (data != nullptr) {
			shaderIn.read((char*)data, size);
		}
	}
}

Shader::~Shader() {
	if (data != nullptr) {
		free(data);
	}
}
