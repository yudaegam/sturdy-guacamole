#pragma once
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <vector>
#include <string>

namespace tinygltf
{
	class Model;
	struct Primitive;
}

namespace sturdy_guacamole
{
	class GLTFModel;
	class GLTFMaterial;

	class GLTFPrimitive
	{
	public:
		GLTFPrimitive(const tinygltf::Model& tinyModel, const tinygltf::Primitive& primitive, const GLTFModel& myModel);
		~GLTFPrimitive() = default;
		void Draw(ID3D11DeviceContext* pDeviceContext) const;
		void DrawInstanced(ID3D11DeviceContext* pDeviceContext, UINT instanceCount);


	public:
		const GLTFMaterial* m_pMaterial{};
		
		struct // used in ID3D11DeviceContext::IASetIndexBuffer();
		{
			ID3D11Buffer* pBuffer{};
			DXGI_FORMAT format = DXGI_FORMAT_R16_UINT; // or DXGI_FORMAT_R32_UINT
			UINT offset{};

			// used in ID3D11DeviceContext::DrawIndexed();
			UINT count{};	
		} m_index;
	
		
		struct // used in ID3D11DeviceContext::IASetVertexBuffers();
		{
			std::vector<ID3D11Buffer*> pBuffers{};
			std::vector<UINT> strides{};
			std::vector<UINT> offsets{};
			UINT count{};
			UINT baseVertexLocation{};
		} m_vertex;

		// inform that the primitive has certain attributes
		ComPtr<ID3D11Buffer> m_cbuffer_attribute{};

		// used in ID3D11Device::IASetInputLayout();
		ComPtr<ID3D11InputLayout> m_inputLayout{};


		// used in ID3D11DeviceContext::IASetPrimitiveTopology();
		D3D11_PRIMITIVE_TOPOLOGY m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	private:
		void ProcessIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const GLTFModel& myModel);
		void ProcessAttributes(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const GLTFModel& myModel);
		void AddVertexBuffer(ID3D11Buffer* pBuffer, UINT stride, UINT offset);
		UINT FindVertexBuffer(const ID3D11Buffer* pBuffer, UINT stride, UINT offset) const;
		UINT FindOrAddVertexBuffer(ID3D11Buffer* pBuffer, UINT stride, UINT offset);
	};
}

