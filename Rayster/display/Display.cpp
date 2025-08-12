#include "Display.hpp"
#include <iostream>

using Microsoft::WRL::ComPtr;

Display Display::instance;
ComPtr<ID3D12Device14> Display::device;

void printGpu(DXGI_ADAPTER_DESC3& desc);

void Display::initialize() {
	ComPtr<IDXGIFactory7> factory;
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter4> adapter;
	DXGI_ADAPTER_DESC3 adapterDesc{};
	ThrowIfFailed(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
	ThrowIfFailed(adapter->GetDesc3(&adapterDesc));

	std::cout << "Chosen GPU:" << std::endl;
	printGpu(adapterDesc);

	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&instance.device)));

	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	ThrowIfFailed(instance.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&instance.commandQueue)));
	ThrowIfFailed(instance.device->CreateFence(instance.fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&instance.fence)));

	instance.fenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (instance.fenceEvent == nullptr) {
		throw "Unable to create fence event.";
	}

	ThrowIfFailed(instance.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&instance.commandAllocator)));
	ThrowIfFailed(instance.device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&instance.commandList)));

	const wchar_t NAME[] = L"Rayster";
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = Display::onWindowMessage;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"RaysterWindowClass";
	wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

	instance.windowClass = RegisterClassExW(&wc);
	if (instance.windowClass == 0) {
		throw "Error in creating Window Class.";
	}

	POINT pos{};
	GetCursorPos(&pos);
	HMONITOR monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitorInfo{};
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfoW(monitor, &monitorInfo);

	instance.window = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW,
		(LPCWSTR)instance.windowClass,
		L"Rayster",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		monitorInfo.rcWork.left + 100,
		monitorInfo.rcWork.top + 100,
		instance.width,
		instance.height,
		nullptr,
		nullptr,
		GetModuleHandle(nullptr),
		nullptr
	);

	if (instance.window == nullptr) {
		throw "Error in creating Window.";
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = instance.width;
	swapChainDesc.Height = instance.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = (UINT)instance.FRAME_COUNT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc{};
	swapChainFullScreenDesc.Windowed = true;

	ComPtr<IDXGISwapChain1> sc;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(instance.commandQueue.Get(), instance.window, &swapChainDesc, &swapChainFullScreenDesc, nullptr, &sc));
	ThrowIfFailed(sc.As(&instance.swapChain));

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = FRAME_COUNT;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(instance.device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&instance.rtvDescriptorHeap)));

	auto firstHandle = instance.rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto handleIncrement = instance.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (size_t i = 0; i < instance.FRAME_COUNT; ++i) {
		instance.rtvHandles[i] = firstHandle;
		instance.rtvHandles[i].ptr += handleIncrement * i;
	}

	instance.loadRtvBuffers();

	D3D12_DESCRIPTOR_HEAP_DESC depthStencilDescriptorHeapDescription{};
	depthStencilDescriptorHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	depthStencilDescriptorHeapDescription.NodeMask = 0;
	depthStencilDescriptorHeapDescription.NumDescriptors = 1;
	depthStencilDescriptorHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(instance.device->CreateDescriptorHeap(&depthStencilDescriptorHeapDescription, IID_PPV_ARGS(&instance.depthStencilDescriptorHeap)));
	instance.depthStencilHandle = instance.depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	instance.loadDepthStencilBuffer(instance.width, instance.height);
}

void Display::cleanUp() {
	instance.freeDepthStencilBuffer();
	instance.depthStencilDescriptorHeap.Reset();

	instance.freeRtvBuffers();
	instance.rtvDescriptorHeap.Reset();
	instance.swapChain.Reset();

	if (instance.window) {
		DestroyWindow(instance.window);
	}

	if (instance.windowClass) {
		UnregisterClassW((LPCWSTR)instance.windowClass, GetModuleHandle(nullptr));
	}

	instance.commandList.Reset();
	instance.commandAllocator.Reset();

	if (instance.fenceEvent != nullptr) {
		CloseHandle(instance.fenceEvent);
	}

	instance.fence.Reset();
	instance.commandQueue.Reset();
	instance.device.Reset();
}

void Display::loadRtvBuffers() {
	for (UINT i = 0; i < instance.FRAME_COUNT; i++) {
		ThrowIfFailed(instance.swapChain->GetBuffer(i, IID_PPV_ARGS(&instance.rtvBuffers[i])));

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		instance.device->CreateRenderTargetView(instance.rtvBuffers[i].Get(), &rtvDesc, instance.rtvHandles[i]);
	}
}

void Display::freeRtvBuffers() {
	for (UINT i = 0; i < instance.FRAME_COUNT; i++) {
		instance.rtvBuffers[i].Reset();
	}
}

void Display::resize() {
	RECT cr;
	if (GetClientRect(instance.window, &cr)) {
		UINT width = cr.right - cr.left;
		UINT height = cr.bottom - cr.top;

		instance.freeRtvBuffers();
		instance.swapChain->ResizeBuffers((UINT)instance.FRAME_COUNT, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
		instance.loadRtvBuffers();
		
		instance.freeDepthStencilBuffer();
		instance.loadDepthStencilBuffer(width, height);
		
		instance.width = width;
		instance.height = height;
		instance.resized = false;
	}
}

void Display::loadDepthStencilBuffer(UINT width, UINT height) {
	D3D12_HEAP_PROPERTIES depthStencilHeapProperties{};
	depthStencilHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthStencilHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthStencilHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	depthStencilHeapProperties.CreationNodeMask = 0;
	depthStencilHeapProperties.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC depthStencilResourceDescription{};
	depthStencilResourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilResourceDescription.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	depthStencilResourceDescription.Width = width;
	depthStencilResourceDescription.Height = height;
	depthStencilResourceDescription.DepthOrArraySize = 1;
	depthStencilResourceDescription.MipLevels = 1;
	depthStencilResourceDescription.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilResourceDescription.SampleDesc.Count = 1;
	depthStencilResourceDescription.SampleDesc.Quality = 0;
	depthStencilResourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilResourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthStencilClearValue{};
	depthStencilClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilClearValue.DepthStencil.Depth = 1.0f;
	depthStencilClearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(instance.device->CreateCommittedResource(
		&depthStencilHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilResourceDescription,
		D3D12_RESOURCE_STATE_DEPTH_READ,
		&depthStencilClearValue,
		IID_PPV_ARGS(&instance.depthStencilBuffer)
	));

	instance.device->CreateDepthStencilView(instance.depthStencilBuffer.Get(), nullptr, instance.depthStencilHandle);
}

void Display::freeDepthStencilBuffer() {
	instance.depthStencilBuffer.Reset();
}

bool Display::poll() {
	MSG message;
	while (PeekMessageW(&message, instance.window, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}

	if (instance.resized) {
		instance.flushPipeline();
		instance.resize();
	}

	return !instance.closed;
}

void Display::signalAndWait() {
	instance.commandQueue->Signal(instance.fence.Get(), ++instance.fenceValue);
	if (SUCCEEDED(instance.fence->SetEventOnCompletion(instance.fenceValue, instance.fenceEvent))) {
		if (WaitForSingleObject(instance.fenceEvent, 20000) != WAIT_OBJECT_0) {
			std::exit(-1);
		}
	} else {
		std::exit(-1);
	}
}

void Display::setFullscreen(bool enabled) {
	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	DWORD exStyle = WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW;

	if (enabled) {
		style = WS_POPUP | WS_VISIBLE;
		exStyle = WS_EX_APPWINDOW;
	}

	SetWindowLongW(instance.window, GWL_STYLE, style);
	SetWindowLongW(instance.window, GWL_EXSTYLE, exStyle);

	if (enabled) {
		HMONITOR monitor = MonitorFromWindow(instance.window, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo{};
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (GetMonitorInfoW(monitor, &monitorInfo)) {
			SetWindowPos(
				instance.window,
				nullptr,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_NOZORDER
			);
		}
	} else {
		ShowWindow(instance.window, SW_MAXIMIZE);
	}

	instance.fullscreened = enabled;
}

void Display::beginFrame(ID3D12GraphicsCommandList10* commandList) {
	instance.currentBufferIndex = instance.swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER rtvBarrier{};
	rtvBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rtvBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rtvBarrier.Transition.pResource = instance.rtvBuffers[instance.currentBufferIndex].Get();
	rtvBarrier.Transition.Subresource = 0;
	rtvBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	rtvBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	D3D12_RESOURCE_BARRIER depthStencilBarrier{};
	depthStencilBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	depthStencilBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	depthStencilBarrier.Transition.pResource = instance.depthStencilBuffer.Get();
	depthStencilBarrier.Transition.Subresource = 0;
	depthStencilBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_READ;
	depthStencilBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	commandList->ResourceBarrier(1, &rtvBarrier);
	commandList->ResourceBarrier(1, &depthStencilBarrier);
	FLOAT clearColor[] = { 0.4f, 0.4f, 0.8f, 1.0f };
	commandList->ClearRenderTargetView(instance.rtvHandles[instance.currentBufferIndex], clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(instance.depthStencilHandle, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
	commandList->OMSetRenderTargets(1, &instance.rtvHandles[instance.currentBufferIndex], false, &instance.depthStencilHandle);
}

void Display::endFrame(ID3D12GraphicsCommandList10* commandList) {
	D3D12_RESOURCE_BARRIER rtvBarrier{};
	rtvBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rtvBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rtvBarrier.Transition.pResource = instance.rtvBuffers[instance.currentBufferIndex].Get();
	rtvBarrier.Transition.Subresource = 0;
	rtvBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	rtvBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	D3D12_RESOURCE_BARRIER depthStencilBarrier{};
	depthStencilBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	depthStencilBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	depthStencilBarrier.Transition.pResource = instance.depthStencilBuffer.Get();
	depthStencilBarrier.Transition.Subresource = 0;
	depthStencilBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	depthStencilBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_READ;

	commandList->ResourceBarrier(1, &rtvBarrier);
	commandList->ResourceBarrier(1, &depthStencilBarrier);
}

void Display::present() {
	instance.swapChain->Present(1, 0);
}

ID3D12GraphicsCommandList10* Display::initializeCommandList() {
	instance.commandAllocator->Reset();
	instance.commandList->Reset(instance.commandAllocator.Get(), nullptr);
	return instance.commandList.Get();
}

void Display::executeCommandList() {
	if (SUCCEEDED(instance.commandList->Close())) {
		ID3D12CommandList* lists[] = { instance.commandList.Get() };
		instance.commandQueue->ExecuteCommandLists(1, lists);
		instance.signalAndWait();
	}
}

HRESULT Display::allocateVertexGpuBuffer(UINT64 size, const IID& riidResource, void** ppvResource) {
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0;
	heapProperties.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resourceDescription{};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDescription.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDescription.Width = size;
	resourceDescription.Height = 1;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 1;
	resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
	resourceDescription.SampleDesc.Count = 1;
	resourceDescription.SampleDesc.Quality = 0;
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;
	
	// Possible optimization to avoid zeroing out memory using D3D12_HEAP_FLAG_CREATE_NOT_ZEROED.
	return instance.device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_COMMON, nullptr, riidResource, ppvResource);
}

HRESULT Display::allocateUploadBuffer(UINT64 size, const IID& riidResource, void** ppvResource) {
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0;
	heapProperties.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resourceDescription{};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDescription.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	resourceDescription.Width = size;
	resourceDescription.Height = 1;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 1;
	resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
	resourceDescription.SampleDesc.Count = 1;
	resourceDescription.SampleDesc.Quality = 0;
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;

	return instance.device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, D3D12_RESOURCE_STATE_COMMON, nullptr, riidResource, ppvResource);
}

LRESULT CALLBACK Display::onWindowMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CLOSE:
			instance.closed = true;
			break;

		case WM_SIZE:
			if (lParam && (HIWORD(lParam) != instance.height || LOWORD(lParam) != instance.width)) {
				instance.resized = true;
			}
			break;

		case WM_KEYDOWN:
			if (wParam == VK_F11) {
				instance.setFullscreen(!instance.fullscreened);
			}
			break;
	}

	return DefWindowProc(wnd, msg, wParam, lParam);
}

static void printGpu(DXGI_ADAPTER_DESC3& desc) {
	std::wcout
		<< "Description:                     " << desc.Description
		<< "\nVendor ID:                       " << desc.VendorId
		<< "\nDevice ID:                       " << desc.DeviceId
		<< "\nSubsystem ID:                    " << desc.SubSysId
		<< "\nRevision:                        " << desc.Revision
		<< "\nDedicated Video Memory:          " << desc.DedicatedVideoMemory
		<< "\nDedicated System Memory:         " << desc.DedicatedSystemMemory
		<< "\nShared System Memory:            " << desc.SharedSystemMemory
		<< "\nLUID:                            0x" << std::hex << desc.AdapterLuid.HighPart << desc.AdapterLuid.LowPart
		<< "\nFlags:                           0x" << desc.Flags << std::dec
		<< "\nGraphics Preemption Granularity: " << desc.GraphicsPreemptionGranularity
		<< "\nCompute Preemption Granularity:  " << desc.ComputePreemptionGranularity
		<< "\n" << std::endl;
}