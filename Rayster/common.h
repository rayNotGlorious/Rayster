#pragma once

// Rayster is a single GPU rasterizer framework.

#define NOMINMAX
#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#ifdef _DEBUG
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#endif

// start() is the function defined by the user of the Rayster. 
int start(int argc, char* argv[]);

#define ThrowIfFailed(res) { \
	HRESULT errorCode = res; \
	if (errorCode != S_OK) { \
		throw "Error Code " + errorCode; \
	} \
}