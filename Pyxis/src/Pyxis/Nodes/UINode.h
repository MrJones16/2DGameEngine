#pragma once

#include <vector>
#include <Pyxis/Renderer/Renderer2D.h>
#include <Pyxis/Nodes/Node.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Pyxis
{
	namespace UI
	{
		/// <summary>
		/// A Base UI Node that can act as a root
		/// </summary>
		class UINode : public Node
		{
		public:
			inline static uint32_t s_MousePressedNodeID = 0;

			UINode(const std::string& name = "UINode") : Node(name) {};
			virtual  ~UINode() = default;

			virtual void OnMousePressed(int mouseButton) {};
			virtual void OnMouseReleased(int mouseButton, bool continuous) {};
			//virtual void OnClick() {};

			//virtual void InspectorRender() override {};
			virtual void OnUpdate(Timestep ts) override {};
			virtual void OnRender() override {};
			
		};
	}

}