#include "Vector.h"

void printVector(DirectX::XMVECTOR vec) {
	printf("(%f, %f, %f, %f)\n", DirectX::XMVectorGetX(vec), DirectX::XMVectorGetY(vec), DirectX::XMVectorGetZ(vec), DirectX::XMVectorGetW(vec));
}
