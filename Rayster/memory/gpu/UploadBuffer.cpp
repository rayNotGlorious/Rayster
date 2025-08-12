#include "UploadBuffer.hpp"

UploadBuffer::UploadBuffer(std::initializer_list<Model> list) : size(0) {
	for (Model model : list) {
		assets.push_back(model);
		size += model.getTotalSize();
	}
	
	ThrowIfFailed(Display::allocateUploadBuffer(size, IID_PPV_ARGS(&buffer)));

	D3D12_RANGE range{};
	range.Begin = 0;
	range.End = size;
	BYTE* bufferAddress = nullptr;
	ThrowIfFailed(buffer->Map(0, &range, (void**)&bufferAddress));

	size_t current = 0;
	for (Model& model : assets) {
		memcpy(bufferAddress + current, model.getVertices(), model.getVertexSize());
		memcpy(bufferAddress + current + model.getVertexSize(), model.getIndices(), model.getIndicesSize());
		current += model.getVertexSize() + model.getIndicesSize();
	}

	buffer->Unmap(0, &range);
}
