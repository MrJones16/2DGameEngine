#pragma once

//#include "NetworkThreadSafeQueue.h"
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include "NetworkMessage.h"
#include <Pyxis/Core/Log.h>


namespace Pyxis
{
	namespace Network
	{

		class ServerInterface
		{
		public:
			ServerInterface();

			virtual ~ServerInterface();

			/// <summary>
			/// Starts the server. It initializes the SteamNetworkingSockets, 
			/// and listens to the port set at instantiation. 
			/// </summary>
			/// <returns>True if successful</returns>
			bool Start(uint16_t port = 0);

			/// <summary>
			/// The main update loop for the server. It polls incoming messages, connection changes, and waits
			/// </summary>
			void UpdateInterface();
			void Stop();


			void SendStringToClient(HSteamNetConnection conn, const std::string& str);
			void SendStringToAllClients(const std::string& str, HSteamNetConnection except = k_HSteamNetConnection_Invalid);
			//Sending messages is volatile! it adds the ID to the message when sending
			//k_nSteamNetworkingSend_
			void SendMessageToClient(HSteamNetConnection conn, Message& message, int nSendFlags = k_nSteamNetworkingSend_Reliable);
			//k_nSteamNetworkingSend_
			void SendMessageToAllClients(Message& message, HSteamNetConnection except = k_HSteamNetConnection_Invalid, int nSendFlags = k_nSteamNetworkingSend_Reliable);
			//k_nSteamNetworkingSend_
			void SendCompressedStringToClient(HSteamNetConnection conn, std::string& compressedStr, int nSendFlags = k_nSteamNetworkingSend_Reliable);
			//k_nSteamNetworkingSend_
			void SendCompressedStringToAllClients(std::string& compressedStr, HSteamNetConnection except = k_HSteamNetConnection_Invalid, int nSendFlags = k_nSteamNetworkingSend_Reliable);
			bool PollMessage(Ref<Message>& MessageOut);
			
			void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);
			static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);
			void PollConnectionStateChanges();
			

		protected:
			//called when a client connects to the server
			inline virtual bool OnClientConnect(HSteamNetConnection client);
			//called when a client appears to have disconnected
			inline virtual void OnClientDisconnect(HSteamNetConnection client);
			//called when a message arrives
			//virtual void OnMessage(HSteamNetConnection client, Message<T>& msg);

		protected:

			static ServerInterface* s_pCallbackInstance;

			std::unordered_set<HSteamNetConnection> m_ClientsSet;

			SteamNetworkingIPAddr m_hLocalAddress;
			HSteamListenSocket m_hListenSock;
			HSteamNetPollGroup m_hPollGroup;
			ISteamNetworkingSockets* m_pInterface;
			ISteamNetworkingUtils* m_pUtils;

			//thread safe queue for incoming message packets
			//ThreadSafeQueue<OwnedMessage<T>> m_QueueMessagesIn;

			

		};

	}
}