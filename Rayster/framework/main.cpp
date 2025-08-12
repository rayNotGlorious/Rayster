#include "common.h"
#include "display/Display.hpp"

using Microsoft::WRL::ComPtr;

int main(int argc, char* argv[]) {
#ifdef _DEBUG
	ComPtr<ID3D12Debug6> dx12Debug;
	ComPtr<IDXGIDebug1> dxgiDebug;

	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&dx12Debug)));
	ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));

	dx12Debug->EnableDebugLayer();
	dxgiDebug->EnableLeakTrackingForThread();
#endif

	Display::initialize();
	
	int returnValue = start(argc, argv);

	Display::cleanUp();

#ifdef _DEBUG
	OutputDebugStringW(L"DXGI Reports living device objects:\n");
	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	OutputDebugStringW(L"DXGI Report End\n");

	dxgiDebug.Reset();
	dx12Debug.Reset();
#endif

	return returnValue;
}