#include <iostream>
#include <DirectXMath.h>
#include <chrono>
#include "display/Display.hpp"
#include "logic/Shader.hpp"
#include "memory/Model.hpp"
#include "memory/gpu/UploadBuffer.hpp"
#include "logic/Camera.hpp"

using Microsoft::WRL::ComPtr;

struct Vertex {
	float x, y, z;
	float r, g, b;
};

int start(int argc, char* argv[]) {
	Display::setFullscreen(true);
	
	Vertex vertices[] = {
		{ -1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f },
		{ -1.0f, +1.0f, -1.0f, 0.0f, 1.0f, 0.0f },
		{ +1.0f, +1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
		{ +1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f },
		{ -1.0f, -1.0f, +1.0f, 1.0f, 0.0f, 0.0f },
		{ -1.0f, +1.0f, +1.0f, 0.0f, 1.0f, 0.0f },
		{ +1.0f, +1.0f, +1.0f, 0.0f, 0.0f, 1.0f },
		{ +1.0f, -1.0f, +1.0f, 0.0f, 1.0f, 0.0f },
	};
	
	// do not derive normals from these faces
	uint32_t indices[] = { 
		// top face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	std::vector<VertexElement> properties = {
		VertexElement("Position", DXGI_FORMAT_R32G32B32_FLOAT, 3 * sizeof(float)),
		VertexElement("Color", DXGI_FORMAT_R32G32B32_FLOAT, 3 * sizeof(float)),
	};

	Model mesh(BufferFromArray(vertices), BufferFromArray(indices), properties);

	UploadBuffer uploadBuffer = { mesh };

	VertexShader vertex("vertex.hlsl");
	PixelShader pixel("pixel.hlsl");

	ComPtr<ID3D12Resource2> vertexBuffer;
	ThrowIfFailed(Display::allocateVertexGpuBuffer(uploadBuffer.getSize(), IID_PPV_ARGS(&vertexBuffer)));

	auto list = Display::initializeCommandList();
	list->CopyBufferRegion(vertexBuffer.Get(), 0, uploadBuffer.getBuffer(), 0, uploadBuffer.getSize());
	Display::executeCommandList();

	D3D12_ROOT_PARAMETER1 mvpRootParameter{};
	mvpRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	mvpRootParameter.Constants.Num32BitValues = 16;
	mvpRootParameter.Constants.ShaderRegister = 0;
	mvpRootParameter.Constants.RegisterSpace = 0;
	mvpRootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription{};
	rootSignatureDescription.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignatureDescription.Desc_1_1.NumParameters = 1;
	rootSignatureDescription.Desc_1_1.pParameters = &mvpRootParameter;
	rootSignatureDescription.Desc_1_1.NumStaticSamplers = 0;
	rootSignatureDescription.Desc_1_1.pStaticSamplers = nullptr;
	rootSignatureDescription.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> rootSignatureErrorBlob;
	ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDescription, &rootSignatureBlob, &rootSignatureErrorBlob));

	ComPtr<ID3D12RootSignature> rootSignature;
	ThrowIfFailed(Display::getDevice().CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
	pipelineDesc.pRootSignature = rootSignature.Get();
	pipelineDesc.VS.BytecodeLength = vertex.getBytecodeLength();
	pipelineDesc.VS.pShaderBytecode = vertex.getBytecode();
	pipelineDesc.PS.BytecodeLength = pixel.getBytecodeLength();
	pipelineDesc.PS.pShaderBytecode = pixel.getBytecode();
	pipelineDesc.DS.BytecodeLength = 0;
	pipelineDesc.DS.pShaderBytecode = nullptr;
	pipelineDesc.HS.BytecodeLength = 0;
	pipelineDesc.HS.pShaderBytecode = nullptr;
	pipelineDesc.GS.BytecodeLength = 0;
	pipelineDesc.GS.pShaderBytecode = nullptr;
	pipelineDesc.StreamOutput.NumEntries = 0;
	pipelineDesc.StreamOutput.NumStrides = 0;
	pipelineDesc.StreamOutput.pBufferStrides = nullptr;
	pipelineDesc.StreamOutput.pSODeclaration = nullptr;
	pipelineDesc.StreamOutput.RasterizedStream = 0;
	pipelineDesc.BlendState.AlphaToCoverageEnable = FALSE;
	pipelineDesc.BlendState.IndependentBlendEnable = FALSE;
	pipelineDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
	pipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	pipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	pipelineDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	pipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	pipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	pipelineDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	pipelineDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
	pipelineDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	pipelineDesc.SampleMask = 0xFFFFFFFF;
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pipelineDesc.RasterizerState.FrontCounterClockwise = FALSE;
	pipelineDesc.RasterizerState.DepthBias = 0;
	pipelineDesc.RasterizerState.DepthBiasClamp = 0.0f;
	pipelineDesc.RasterizerState.DepthClipEnable = FALSE;
	pipelineDesc.RasterizerState.MultisampleEnable = FALSE;
	pipelineDesc.RasterizerState.AntialiasedLineEnable = FALSE;
	pipelineDesc.RasterizerState.ForcedSampleCount = 0;
	pipelineDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	pipelineDesc.DepthStencilState.DepthEnable = TRUE;
	pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	pipelineDesc.DepthStencilState.StencilEnable = FALSE;
	pipelineDesc.DepthStencilState.StencilReadMask = 0;
	pipelineDesc.DepthStencilState.StencilWriteMask = 0;
	pipelineDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pipelineDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	pipelineDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	pipelineDesc.InputLayout.NumElements = mesh.getElementPropertyCount();
	pipelineDesc.InputLayout.pInputElementDescs = mesh.getElementProperties();
	pipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.NumRenderTargets = 1;
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineDesc.SampleDesc.Count = 1;
	pipelineDesc.SampleDesc.Quality = 0;
	pipelineDesc.NodeMask = 0;
	pipelineDesc.CachedPSO.CachedBlobSizeInBytes = 0;
	pipelineDesc.CachedPSO.pCachedBlob = nullptr;
	pipelineDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	ComPtr<ID3D12PipelineState> pipelineStateObject;
	ThrowIfFailed(Display::getDevice().CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineStateObject)));

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = _countof(vertices) * sizeof(Vertex);
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	indexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress() + sizeof(vertices);
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = sizeof(indices);
	
	Camera camera{ 
		DirectX::XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f), 
		DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), 
		0.05f
	};

	float angle = 0.0f;
	while (Display::poll()) {
		DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f), Display::getAspectRatio(), 0.1f, 100.0f);
		DirectX::XMMATRIX view{ camera.deriveViewMatrix() };
		DirectX::XMMATRIX model = DirectX::XMMatrixRotationY(angle);
		angle += 0.001f;

		DirectX::XMMATRIX mvp = DirectX::XMMatrixTranspose(model * view * projection);		

		auto list = Display::initializeCommandList();
		Display::beginFrame(list);

		list->SetPipelineState(pipelineStateObject.Get());
		list->SetGraphicsRootSignature(rootSignature.Get());

		list->IASetVertexBuffers(0, 1, &vertexBufferView);
		list->IASetIndexBuffer(&indexBufferView);
		list->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		list->SetGraphicsRoot32BitConstants(0, 16, &mvp, 0);

		D3D12_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (FLOAT)Display::getWidth();
		viewport.Height = (FLOAT)Display::getHeight();
		viewport.MinDepth = 1.0f;
		viewport.MaxDepth = 0.0f;
		list->RSSetViewports(1, &viewport);

		D3D12_RECT scissorRectangle{};
		scissorRectangle.top = 0;
		scissorRectangle.left = 0;
		scissorRectangle.right = Display::getWidth();
		scissorRectangle.bottom = Display::getHeight();
		list->RSSetScissorRects(1, &scissorRectangle);

		list->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);
		
		Display::endFrame(list);
		Display::executeCommandList();
		Display::present();
	}

	Display::flushPipeline();
	return 0;
}