#pragma once

#include "RendererAPI.h"

namespace Pyxis
{
	class RenderCommand
	{
	public:
		//render commands should not do multiple things

		inline static void Init()
		{
			s_RendererAPI->Init();
		}

		inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		inline static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		inline static void Clear()
		{
			s_RendererAPI->Clear();
		}

		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			vertexArray->Bind();
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		inline static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			vertexArray->Bind();
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}

	private:
		static RendererAPI* s_RendererAPI;
	};
}