#include "SingleplayerGameLayer.h"
#include <Pyxis/Nodes/UI.h>

namespace Pyxis
{

	SingleplayerGameLayer::SingleplayerGameLayer()
	{
		
	}

	SingleplayerGameLayer::~SingleplayerGameLayer()
	{

	}

	void SingleplayerGameLayer::Start()
	{
		CreateWorld();
	}

	void SingleplayerGameLayer::OnUpdate(Timestep ts)
	{
		PROFILE_SCOPE("GameLayer::OnUpdate");

		m_OrthographicCameraController.OnUpdate(ts);
		m_Scene->Update(ts);

		//rendering
		#if STATISTICS
		Renderer2D::ResetStats();
		#endif

		{
			PROFILE_SCOPE("Renderer Prep");
			m_SceneFrameBuffer->Bind();
			RenderCommand::SetClearColor({ 198 / 255.0f, 239 / 255.0f, 249 / 255.0f, 1 });
			RenderCommand::Clear();
			uint32_t clear = 0;
			m_SceneFrameBuffer->ClearColorAttachment(1, &clear);
			Renderer2D::BeginScene(m_OrthographicCameraController.GetCamera().get());
		}

		
		//only run per tick rate
		auto time = std::chrono::high_resolution_clock::now();
		if (m_TickRate > 0 &&
			std::chrono::time_point_cast<std::chrono::microseconds>(time).time_since_epoch().count()
			-
			std::chrono::time_point_cast<std::chrono::microseconds>(m_UpdateTime).time_since_epoch().count()
			>= (1.0f / m_TickRate) * 1000000.0f)
		{
			PROFILE_SCOPE("Simulation Update");

			m_UpdateTime = time;
			//for singleplayer, just construct your own merged tick and 
			//handle it
			MergedTickClosure tc;
			tc.AddTickClosure(m_CurrentTickClosure, 0);
			HandleTickClosure(tc);

			//reset tick closure
			m_CurrentTickClosure = TickClosure();
		}

		GameUpdate(ts);


		m_Scene->Render();

		Renderer2D::EndScene();

		auto mp = GetMousePositionImGui();
		//flip the y so bottom left is 0,0
		mp.y = m_ViewportSize.y - mp.y;
		if (mp.x >= 0 && mp.x < m_ViewportSize.x && mp.y >= 0 && mp.y < m_ViewportSize.y)
		{
			uint32_t nodeID; m_SceneFrameBuffer->ReadPixel(1, mp.x, mp.y, &nodeID);
			m_Scene->m_HoveredNodeID = nodeID;
		}

		//auto mp = GetMousePositionImGui();
		////flip the y so bottom left is 0,0
		//mp.y = m_ViewportSize.y - mp.y;
		//if (mp.x >= 0 && mp.x < m_ViewportSize.x && mp.y >= 0 && mp.y < m_ViewportSize.y)
		//{
		//	//we are in bounds, so lets read the pixel
		//	//PX_TRACE("Mouse Pos: {0}, {1}", mp.x, mp.y);
		//	uint32_t pixel; m_SceneFrameBuffer->ReadPixel(1, mp.x, mp.y, &pixel);
		//	PX_TRACE("ID of Hovered Entity: {0}", pixel);
		//	if (Pyxis::Input::IsMouseButtonPressed(PX_MOUSE_BUTTON_LEFT))
		//	{
		//		if (SceneHierarchyPanel* panel = dynamic_cast<SceneHierarchyPanel*>(m_Panels[1].get()))
		//		{
		//			// do Child specific stuff
		//			if (pixel == 0)
		//			{
		//				PX_TRACE("Set selected node to null");
		//				panel->SetSelectedNode(nullptr);
		//			}
		//			else
		//			{
		//				PX_TRACE("Set selected node to object! {0}", pixel);
		//				PX_TRACE("It's id is: {0}", Node::Nodes[pixel]->GetID());
		//				panel->SetSelectedNode(Node::Nodes[pixel]->shared_from_this());
		//			}
		//			
		//		}
		//	}
		//}

		m_SceneFrameBuffer->Unbind();
	}

	void SingleplayerGameLayer::OnImGuiRender()
	{
		auto dock = ImGui::DockSpaceOverViewport(ImGui::GetID("MainDock"), (const ImGuiViewport*)0, ImGuiDockNodeFlags_PassthruCentralNode);
		ClientImGuiRender(dock);
	}

	

}