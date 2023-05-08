#include "GLTFPrimitive.h"
#include "GLTFModel.h"
#include "GLTFDx11Helpers.h"

#include "Dx11Application.h"
#include "Dx11Helpers.h"
#include "Graphics.h"

#include <tiny_gltf.h>

#include <iostream>

sturdy_guacamole::GLTFPrimitive::GLTFPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const GLTFModel& myModel)
{
	m_topology = sturdy_guacamole::ConvertToDx11Topology(primitive.mode);
	ProcessIndices(model, primitive, myModel);
	ProcessAttributes(model, primitive, myModel);

	if (static_cast<size_t>(primitive.material) < myModel.m_materials.size())
		m_pMaterial = &myModel.m_materials[primitive.material];
}

void sturdy_guacamole::GLTFPrimitive::Draw(ID3D11DeviceContext* pDeviceContext) const
{
	pDeviceContext->IASetInputLayout(m_inputLayout.Get());
	pDeviceContext->IASetIndexBuffer(m_index.pBuffer, m_index.format, m_index.offset);
	pDeviceContext->IASetVertexBuffers(0, (UINT)m_vertex.pBuffers.size(), m_vertex.pBuffers.data(), m_vertex.strides.data(), m_vertex.offsets.data());

	if (m_pMaterial != nullptr)
	{
		pDeviceContext->PSSetSamplers(0, (UINT)m_pMaterial->m_pSamplers.size(), m_pMaterial->m_pSamplers.data());
		pDeviceContext->PSSetShaderResources(0, (UINT)m_pMaterial->m_pSRViews.size(), m_pMaterial->m_pSRViews.data());
		pDeviceContext->PSSetConstantBuffers(0, 1, m_pMaterial->m_cbuffer_material.GetAddressOf());
	}
	if (m_index.pBuffer != nullptr)
	{
		pDeviceContext->IASetIndexBuffer(m_index.pBuffer, m_index.format, m_index.offset);
		pDeviceContext->DrawIndexed(m_index.count, 0, 0);
	}
	else
	{
		pDeviceContext->Draw(m_vertex.count, 0);
	}
}

void sturdy_guacamole::GLTFPrimitive::DrawInstanced(ID3D11DeviceContext* pDeviceContext, UINT instanceCount)
{
	pDeviceContext->IASetInputLayout(m_inputLayout.Get());
	pDeviceContext->IASetVertexBuffers(0, (UINT)m_vertex.pBuffers.size(), m_vertex.pBuffers.data(), m_vertex.strides.data(), m_vertex.offsets.data());
	pDeviceContext->PSSetSamplers(0, (UINT)m_pMaterial->m_pSamplers.size(), m_pMaterial->m_pSamplers.data());
	pDeviceContext->PSSetShaderResources(0, (UINT)m_pMaterial->m_pSRViews.size(), m_pMaterial->m_pSRViews.data());
	pDeviceContext->PSSetConstantBuffers(0, 1, m_pMaterial->m_cbuffer_material.GetAddressOf());
	pDeviceContext->PSSetConstantBuffers(2, 1, m_cbuffer_attribute.GetAddressOf());

	if (m_index.pBuffer != nullptr)
	{
		pDeviceContext->IASetIndexBuffer(m_index.pBuffer, m_index.format, m_index.offset);
		pDeviceContext->DrawIndexedInstanced(m_index.count, instanceCount, 0, 0, 0);
	}
	else
	{
		pDeviceContext->DrawInstanced(m_vertex.count, instanceCount, 0, 0);
	}
}

void sturdy_guacamole::GLTFPrimitive::ProcessIndices(const tinygltf::Model& tinyModel, const tinygltf::Primitive& primitive, const GLTFModel& myModel)
{
	if (static_cast<size_t>(primitive.indices) < tinyModel.accessors.size())
	{
		const auto& accessor = tinyModel.accessors[primitive.indices];
		const int bufferViewIdx = accessor.bufferView;

		m_index.pBuffer = myModel.m_bufferViews[bufferViewIdx].m_buffer.Get();
		m_index.format = (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) ?
			DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

		m_index.offset = accessor.byteOffset;
		m_index.count = (UINT)accessor.count;
	}
}

void sturdy_guacamole::GLTFPrimitive::ProcessAttributes(const tinygltf::Model& tinyModel, const tinygltf::Primitive& primitive, const GLTFModel& myModel)
{
	enum ATTRIBUTE { POSITION, NORMAL, TEXCOORD, ATTRIBUTE_COUNT };

	// used in ID3D11Device::CreateInputLayout();
	std::vector<D3D11_INPUT_ELEMENT_DESC> ieDescs{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 15, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 15, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	  15, 0 },
	};

	struct
	{
		BOOL HasNormal{};
		BOOL HasTangent{};
		BOOL HasTexcoord_0{};
		BOOL HasColor_0{};
		BOOL HasJoint_0{};
		BOOL HasWeight_0{};
		BOOL padding[2]{};
	} attributeConstants;

	const auto& attributes = primitive.attributes;

	{
		const int accessor_idx = attributes.at("POSITION");
		const auto& accessor = tinyModel.accessors[accessor_idx];
		const int byteStride = accessor.ByteStride(tinyModel.bufferViews[accessor.bufferView]);
		const auto& myBufView = myModel.m_bufferViews[accessor.bufferView];

		m_vertex.count = (UINT)accessor.count;

		UINT offset{};
		UINT alignedOffset{ (UINT)accessor.byteOffset };
		if (byteStride < (UINT)accessor.byteOffset)
		{
			offset = (UINT)accessor.byteOffset;
			alignedOffset = 0;
		}

		const UINT inputSlot = this->FindOrAddVertexBuffer(myBufView.m_buffer.Get(), byteStride, offset);
		ieDescs[POSITION].InputSlot = inputSlot;
		ieDescs[POSITION].AlignedByteOffset = alignedOffset;
	}

	if (attributes.contains("NORMAL"))
	{
		attributeConstants.HasNormal = true;

		const int accessor_idx = attributes.at("NORMAL");
		const auto& accessor = tinyModel.accessors[accessor_idx];
		const int byteStride = accessor.ByteStride(tinyModel.bufferViews[accessor.bufferView]);
		const auto& myBufView = myModel.m_bufferViews[accessor.bufferView];

		UINT offset{};
		UINT alignedOffset{ (UINT)accessor.byteOffset };
		if (byteStride < (UINT)accessor.byteOffset)
		{
			offset = (UINT)accessor.byteOffset;
			alignedOffset = 0;
		}

		const UINT inputSlot = this->FindOrAddVertexBuffer(myBufView.m_buffer.Get(), byteStride, offset);
		ieDescs[NORMAL].InputSlot = inputSlot;
		ieDescs[NORMAL].AlignedByteOffset = alignedOffset;
	}

	if (attributes.contains("TEXCOORD_0"))
	{
		attributeConstants.HasTexcoord_0 = true;

		const int accessor_idx = attributes.at("TEXCOORD_0");
		const auto& accessor = tinyModel.accessors[accessor_idx];
		const int byteStride = accessor.ByteStride(tinyModel.bufferViews[accessor.bufferView]);
		const auto& myBufView = myModel.m_bufferViews[accessor.bufferView];

		const DXGI_FORMAT format = sturdy_guacamole::ConvertToDXGIFormat(accessor.componentType, 2);

		UINT offset{};
		UINT alignedOffset{ (UINT)accessor.byteOffset };
		if (byteStride < (UINT)accessor.byteOffset)
		{
			offset = (UINT)accessor.byteOffset;
			alignedOffset = 0;
		}

		const UINT inputSlot = this->FindOrAddVertexBuffer(myBufView.m_buffer.Get(), byteStride, offset);
		ieDescs[TEXCOORD].Format = format;
		ieDescs[TEXCOORD].InputSlot = inputSlot;
		ieDescs[TEXCOORD].AlignedByteOffset = alignedOffset;
	}


	// create input layout
	const auto& vsBlob = Graphics::Get().m_vtxShader.basic_blob;
	ThrowIfFailed(g_pDevice->CreateInputLayout(ieDescs.data(), (UINT)ieDescs.size(),
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout));

	// create attribute constants buffer
	CD3D11_BUFFER_DESC bufDesc{ sizeof(attributeConstants), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_IMMUTABLE };
	D3D11_SUBRESOURCE_DATA initData{ &attributeConstants };
	ThrowIfFailed(g_pDevice->CreateBuffer(&bufDesc, &initData, &m_cbuffer_attribute));
}

void sturdy_guacamole::GLTFPrimitive::AddVertexBuffer(ID3D11Buffer* pBuffer, UINT stride, UINT offset)
{
	m_vertex.pBuffers.push_back(pBuffer);
	m_vertex.offsets.push_back(offset);
	m_vertex.strides.push_back(stride);
}

UINT sturdy_guacamole::GLTFPrimitive::FindVertexBuffer(const ID3D11Buffer* pBuffer, UINT stride, UINT offset) const
{
	UINT inputSlot{};
	for (; inputSlot < m_vertex.pBuffers.size(); ++inputSlot)
	{
		if (m_vertex.pBuffers[inputSlot] == pBuffer && m_vertex.offsets[inputSlot] == offset && m_vertex.strides[inputSlot] == stride)
			break;
	}

	return inputSlot;
}

UINT sturdy_guacamole::GLTFPrimitive::FindOrAddVertexBuffer(ID3D11Buffer* pBuffer, UINT stride, UINT offset)
{
	const UINT inputSlot = FindVertexBuffer(pBuffer, stride, offset);
	if (inputSlot == m_vertex.pBuffers.size())
		AddVertexBuffer(pBuffer, stride, offset);

	return inputSlot;
}
