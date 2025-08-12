#pragma once
#include <common.h>
#include <vector>

#define BufferFromArray(array) (Buffer((void*) array, sizeof(array)))

struct Buffer {
	const void* data;
	const size_t size;

	Buffer(const void* data, const size_t size) : data(data), size(size) {}
};

struct VertexElement {
	const char* name;
	const DXGI_FORMAT format;
	const UINT size;

	VertexElement(const char* name, const DXGI_FORMAT format, const UINT size) : name(name), format(format), size(size) {}
};

class Model {
	Buffer vertices;
	Buffer indices;
	std::vector<D3D12_INPUT_ELEMENT_DESC> elementProperties;

public:
	Model(Buffer vertices, Buffer indices, std::vector<VertexElement> properties);
	
	// Gets the total size of the vertex data in the model.
	inline size_t getVertexSize() const {
		return vertices.size;
	}

	inline const void* getVertices() const {
		return vertices.data;
	}

	// Gets the total size of the index data in the model.
	inline size_t getIndicesSize() const {
		return indices.size;
	}

	inline const void* getIndices() const {
		return indices.data;
	}

	// Gets the total size of both the vertex and index data in the model.
	inline size_t getTotalSize() const {
		return getVertexSize() + getIndicesSize();
	}

	inline UINT getElementPropertyCount() const {
		return (UINT) elementProperties.size();
	}

	inline const D3D12_INPUT_ELEMENT_DESC* getElementProperties() const {
		return elementProperties.data();
	}
};