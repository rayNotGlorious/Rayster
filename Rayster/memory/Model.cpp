#include "model.hpp"

Model::Model(Buffer vertices, Buffer indices, std::vector<VertexElement> properties) : vertices(vertices), indices(indices) {
	elementProperties.reserve(properties.size());
	UINT alignment = 0;

	for (VertexElement& property : properties) {
		D3D12_INPUT_ELEMENT_DESC description{
			.SemanticName = property.name,
			.SemanticIndex = 0,
			.Format = property.format,
			.InputSlot = 0,
			.AlignedByteOffset = alignment,
			.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			.InstanceDataStepRate = 0,
		};

		elementProperties.push_back(description);
		alignment += property.size;
	}
}
