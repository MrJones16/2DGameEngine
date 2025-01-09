#pragma once

#include <Pyxis/Core/Log.h>
#include <Pyxis/Core/Core.h>
#include <Pyxis/Nodes/Node.h>
#include <Pyxis/Nodes/UI.h>

//#include "GameLayer.h"
//#include "SingleplayerGameLayer.h"
//#include "MultiplayerGameLayer.h"
//#include "HostingGameLayer.h"
#include "steam/isteamfriends.h"



namespace Pyxis
{
	class MenuNode : public Node
	{
	public:
		MenuNode(const std::string& name = "MenuNode");

		~MenuNode();

		virtual void OnUpdate(Timestep ts) override;
		//virtual void OnImGuiRender() override;
		//virtual void OnEvent(Event& e) override;


		void PlayButton();

		void FailedToConnect();

		//void OnGameLayerStarted(GameLayer& layer);

		

	private:
		//recievers
		Reciever<void()> m_PlayButtonReciever;

		//////////////////////////////////////
		/// Steam Callbacks
		//////////////////////////////////////
		//STEAM_CALLBACK(MultiplayerGameLayer, OnGameRichPresenceJoinRequested, GameRichPresenceJoinRequested_t);
		//void OnGameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* pCallback);
		//void OnGameOverlayActivated(GameOverlayActivated_t* pCallback);

		STEAM_CALLBACK(MenuNode, OnGameRichPresenceJoinRequested, GameRichPresenceJoinRequested_t, m_CallbackRichPresenceJoinRequested);
		STEAM_CALLBACK(MenuNode, OnGameOverlayActivated, GameOverlayActivated_t, m_CallbackGameOverlayActivated);

		//CCallback<MenuLayer, GameRichPresenceJoinRequested_t> m_CallbackRichPresenceJoinRequested;
		//CCallback<MenuLayer, GameOverlayActivated_t> m_CallbackGameOverlayActivated;


		//std::weak_ptr<SingleplayerGameLayer> m_SinglePlayerLayer;
		//std::weak_ptr<MultiplayerGameLayer> m_MultiplayerLayer;
		//std::weak_ptr<HostingGameLayer> m_HostingLayer;

		bool m_AnyGameLayerAttached = false;

		float m_PlayerColor[3] = {0,0,0};
		char m_PlayerName[64] = "name";

		//UI things
		bool m_InMainMenu = true;
		char m_InputAddress[22] = "127.0.0.1:21218";

	};
}