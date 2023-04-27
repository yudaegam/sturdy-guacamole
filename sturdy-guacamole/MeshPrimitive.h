#pragma once
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <vector>

namespace tinygltf
{
	class Model;
	struct Primitive;
}

namespace sturdy_guacamole
{
	// this class deal with the mesh primitive of glTF
	class MeshPrimitive
	{
	public:
		MeshPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive);
		~MeshPrimitive() = default;
		void Draw(ID3D11DeviceContext* pDeviceContext) const;
		void DrawInstanced(ID3D11DeviceContext* pDeviceContext, UINT instanceCount);


	public:
		// used in ID3D11DeviceContext::IASetIndexBuffer();
		ComPtr<ID3D11Buffer> m_indexBuffer{};
		DXGI_FORMAT m_indexBufferFormat = DXGI_FORMAT_R16_UINT; // or DXGI_FORMAT_R32_UINT

		// it would be always 0. once creating index buffer, the offset was already applied
		UINT m_indexBufferOffset{};

		// used in ID3D11DeviceContext::DrawIndexed();
		UINT m_indexCount{};
	
		// used in ID3D11DeviceContext::IASetVertexBuffers();
		ComPtr<ID3D11Buffer> m_vertexBuffer{};
		UINT m_vertexBufferStride{};
		UINT m_vertexBufferOffset{};

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_inputElementDescs{};

		// used in ID3D11DeviceContext::IASetPrimitiveTopology();
		D3D11_PRIMITIVE_TOPOLOGY m_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	private:
		void ProcessIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive);
		void ProcessAttributes(const tinygltf::Model& model, const tinygltf::Primitive& primitive);
		bool SetPrimitiveTopology(const tinygltf::Primitive& primitive);
	};
}
