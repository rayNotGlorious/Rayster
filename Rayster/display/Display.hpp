#pragma once

#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <string>
#include <chrono>
#include <functional>

#include "common.h"
#include "display/Key.hpp"

class Display {
public:
	// Returns whether or not the window is closed.
	static bool poll();
	static void signalAndWait();
	static void setFullscreen(bool enabled);
	static void beginFrame(ID3D12GraphicsCommandList* commandList);
	static void endFrame(ID3D12GraphicsCommandList* commandList);
	static void present();
	static ID3D12GraphicsCommandList* initializeCommandList();
	static void executeCommandList();

	static HRESULT allocateVertexGpuBuffer(UINT64 size, const IID& riidResource, void** ppvResource);
	static HRESULT allocateUploadBuffer(UINT64 size, const IID& riidResource, void** ppvResource);

	static inline void flushPipeline() {
		for (size_t i = 0; i < instance.FRAME_COUNT; i++) {
			instance.signalAndWait();
		}
	}

	static inline ID3D12Device& getDevice() {
		return *instance.device.Get();
	}

	static inline UINT getHeight() {
		return instance.height;
	}

	static inline UINT getWidth() {
		return instance.width;
	}

	static inline FLOAT getAspectRatio() {
		return (float)getWidth() / (float)getHeight();
	}

	static inline float getDeltaTime() {
		return instance.deltaTime.count();
	}

	static void registerKeyPressCallback(Key key, std::function<void(void)> callback, std::optional<std::string> name = std::nullopt);
	static void deregisterKeyPressCallback(Key key, std::string name);
	static void registerKeyReleaseCallback(Key key, std::function<void(void)> callback, std::optional<std::string> name = std::nullopt);
	static void deregisterKeyReleaseCallback(Key key, std::string name);
	static bool isKeyPressed(Key key);

	static void registerFrameCallback(std::function<void(float)> callback, std::string name);
	static void deregisterFrameCallback(const std::string& name);

private:
	static Display instance;

	static void initialize();
	static void cleanUp();
	static void loadRtvBuffers();
	static void freeRtvBuffers();
	static void resize();
	static void loadDepthStencilBuffer(UINT width, UINT height);
	static void freeDepthStencilBuffer();

	static constexpr size_t FRAME_COUNT = 2;

	inline static Display& getInstance() {
		return instance;
	}

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	size_t currentBufferIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12Fence1> fence;
	UINT fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[FRAME_COUNT]{};
	Microsoft::WRL::ComPtr<ID3D12Resource2> rtvBuffers[FRAME_COUNT];

	Microsoft::WRL::ComPtr<ID3D12Resource2> depthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> depthStencilDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle{};

	// Window Info
	ATOM windowClass;
	HWND window;
	UINT width = 1920;
	UINT height = 1080;
	bool closed = false;
	bool resized = false;
	bool fullscreened = false;

	// Keys
	// TODO: Separate this into a seperate "Input" singleton?
	using KeyDirectory = std::unordered_map<Key, std::vector<std::tuple<std::optional<std::string>, std::function<void(void)>>>>;

	KeyDirectory keyPressCallbacks;
	KeyDirectory keyReleaseCallbacks;
	
	std::unordered_set<Key> pressed;
	static LRESULT CALLBACK onWindowMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	static void registerKeyCallback(KeyDirectory& directory, Key key, std::function<void(void)> callback, std::optional<std::string> name);
	static void deregisterKeyCallback(KeyDirectory& directory, Key key, std::string name);
	static void handleKeyCallback(KeyDirectory& directory, Key key);

	// Game State
	using FrameDirectory = std::unordered_map<std::string, std::function<void(float)>>;

	FrameDirectory frameCallbacks;
	std::chrono::duration<float, std::milli> deltaTime;
	static void handleFrameCallbacks(float deltaTime);

public:
	Display(const Display&) = delete;
	Display& operator=(const Display&) = delete;

	friend int main(int argc, char* argv[]);

private:
	Display() = default;
};