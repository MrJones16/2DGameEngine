#include "pxpch.h"

#include "NetworkServer.h"

#ifndef PX_DEFAULT_PORT
#define PX_DEFAULT_PORT 21218
#endif

namespace Pyxis
{
	namespace Network
	{
		
		ServerInterface::ServerInterface()
		{
			
		}

		
		ServerInterface::~ServerInterface()
		{

		}

		
		bool ServerInterface::Start(uint16_t port)
		{
			//initialze steam sockets
			SteamDatagramErrMsg errMsg;
			if (!GameNetworkingSockets_Init(nullptr, errMsg))
			{
				PX_CORE_ERROR("SteamServer::Start->GameNetworkingSockets_Init failed.  {0}", errMsg);
				return false;
			}


			if (port == 0) port = PX_DEFAULT_PORT;
			m_hLocalAddress.Clear();
			m_hLocalAddress.m_port = port;

			// Select instance to use.  For now we'll always use the default.
			// But we could use SteamGameServerNetworkingSockets() on Steam.
			m_pInterface = SteamNetworkingSockets();

			m_pUtils = SteamNetworkingUtils();

			// Start listening
			SteamNetworkingConfigValue_t opt;

			opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);
			m_hListenSock = m_pInterface->CreateListenSocketIP(m_hLocalAddress, 1, &opt);
			if (m_hListenSock == k_HSteamListenSocket_Invalid)
			{
				PX_CORE_ERROR("Failed to listen on port {0}", m_hLocalAddress.m_port);
				return false;
			}
			m_hPollGroup = m_pInterface->CreatePollGroup();
			if (m_hPollGroup == k_HSteamNetPollGroup_Invalid)
			{
				PX_CORE_ERROR("Failed to listen on port {0}", m_hLocalAddress.m_port);
				return false;
			}
			PX_INFO("Steam Server listening on port {0}", m_hLocalAddress.m_port);
			return true;
		}

		
		void ServerInterface::UpdateInterface()
		{
			//PollIncomingMessages();
			PollConnectionStateChanges();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		
		void ServerInterface::Stop()
		{
			// Close all the connections
			PX_CORE_TRACE("Closing connections...");
			for (auto it : m_mapClients)
			{
				// Send them one more goodbye message.  Note that we also have the
				// connection close reason as a place to send final data.  However,
				// that's usually best left for more diagnostic/debug text not actual
				// protocol strings.
				SendStringToClient(it.first, "Server is shutting down.  Goodbye.");

				// Close the connection.  We use "linger mode" to ask SteamNetworkingSockets
				// to flush this out and close gracefully.
				m_pInterface->CloseConnection(it.first, 0, "Server Shutdown", true);
			}
			m_mapClients.clear();

			m_pInterface->CloseListenSocket(m_hListenSock);
			m_hListenSock = k_HSteamListenSocket_Invalid;

			m_pInterface->DestroyPollGroup(m_hPollGroup);
			m_hPollGroup = k_HSteamNetPollGroup_Invalid;

			GameNetworkingSockets_Kill();

			PX_CORE_INFO("[SERVER] Stopped!");

		}

		void ServerInterface::SendStringToClient(HSteamNetConnection conn, const std::string& str)
		{
			Network::Message msg;
			msg.header.id = 0;
			msg.PushData(str.c_str(), str.size());
			SendMessageToClient(conn, msg);
		}

		void ServerInterface::SendStringToAllClients(const std::string& str, HSteamNetConnection except)
		{
			Network::Message msg;
			msg.header.id = 0;
			msg.PushData(str.c_str(), str.size());
			for (auto& c : m_mapClients)
			{
				if (c.first != except)
					SendMessageToClient(c.first, msg);
			}
		}

		void ServerInterface::SendMessageToClient(HSteamNetConnection conn, Message& message)
		{
			message << message.header.id;
			m_pInterface->SendMessageToConnection(conn, message.body.data(), (uint32)message.size(), k_nSteamNetworkingSend_Reliable, nullptr);
		}

		void ServerInterface::SendMessageToAllClients(Message& message, HSteamNetConnection except)
		{
			for (auto& c : m_mapClients)
			{
				if (c.first != except)
					SendMessageToClient(c.first, message);
			}
		}

		void ServerInterface::SendUnreliableMessageToClient(HSteamNetConnection conn, Message& message)
		{
			message << message.header.id;
			m_pInterface->SendMessageToConnection(conn, message.body.data(), (uint32)message.size(), k_nSteamNetworkingSend_Unreliable, nullptr);
		}

		void ServerInterface::SendUnreliableMessageToAllClients(Message& message, HSteamNetConnection except)
		{
			for (auto& c : m_mapClients)
			{
				if (c.first != except)
					SendUnreliableMessageToClient(c.first, message);
			}
		}

		bool ServerInterface::PollMessage(Ref<Message>& MessageOut)
		{
			ISteamNetworkingMessage* pIncomingMsg = nullptr;
			int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup(m_hPollGroup, &pIncomingMsg, 1);
			if (numMsgs == 0) return false;
			if (numMsgs < 0)
			{
				PX_CORE_ERROR("Error checking for messages");
				return false;
			}
			assert(numMsgs == 1 && pIncomingMsg);
			auto itClient = m_mapClients.find(pIncomingMsg->m_conn);
			assert(itClient != m_mapClients.end());

			MessageOut = CreateRef<Message>(pIncomingMsg->m_pData, pIncomingMsg->m_cbSize);
			*MessageOut >> MessageOut->header.id;
			MessageOut->clientID = itClient->second;
			pIncomingMsg->Release();
			return true;
		}
		
		void ServerInterface::PollIncomingMessages()
		{
			while (true)
			{
				ISteamNetworkingMessage* pIncomingMsg = nullptr;
				int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup(m_hPollGroup, &pIncomingMsg, 1);
				if (numMsgs == 0)
					break;
				if (numMsgs < 0)
					PX_CORE_ERROR("Error checking for messages");
				assert(numMsgs == 1 && pIncomingMsg);
				auto itClient = m_mapClients.find(pIncomingMsg->m_conn);
				assert(itClient != m_mapClients.end());

				// '\0'-terminate it to make it easier to parse
				Message px_msg(pIncomingMsg->m_pData, pIncomingMsg->m_cbSize);
				std::string sCmd;
				sCmd.assign((const char*)pIncomingMsg->m_pData, pIncomingMsg->m_cbSize);
				const char* cmd = sCmd.c_str();
				
				px_msg >> px_msg.header.id;
				if (px_msg.header.id == 0)
				{
					std::string px_str = "";
					px_str.assign((const char*)px_msg.body.data(), px_msg.size());
					PX_CORE_TRACE("Message Recieved. ID: {0}, Message: {1}", px_msg.header.id, px_str);
				}else if (px_msg.header.id == 1)
				{
					
					PX_CORE_TRACE("Message Recieved. ID: {0}", px_msg.header.id);
					std::vector<int> resultOfTest;
					px_msg >> resultOfTest;
					PX_TRACE("Here's the vector: ");
					for (int x : resultOfTest)
					{
						PX_TRACE(x);
					}
				}
				

				// We don't need this anymore.
				pIncomingMsg->Release();
			}
		}

		
		void ServerInterface::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
		{
			char temp[1024];

			// What's the state of the connection?
			switch (pInfo->m_info.m_eState)
			{
			case k_ESteamNetworkingConnectionState_None:
				// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
				break;

			case k_ESteamNetworkingConnectionState_ClosedByPeer:
			case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
			{
				// Ignore if they were not previously connected.  (If they disconnected
				// before we accepted the connection.)
				if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
				{

					// Locate the client.  Note that it should have been found, because this
					// is the only codepath where we remove clients (except on shutdown),
					// and connection change callbacks are dispatched in queue order.
					auto itClient = m_mapClients.find(pInfo->m_hConn);
					assert(itClient != m_mapClients.end());

					// Select appropriate log messages
					const char* pszDebugLogAction;
					if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
					{
						pszDebugLogAction = "problem detected locally";
						PX_CORE_WARN("Alas, {0} hath fallen into shadow. ({1})", itClient->second, pInfo->m_info.m_szEndDebug);
					}
					else
					{
						// Note that here we could check the reason code to see if
						// it was a "usual" connection or an "unusual" one.
						pszDebugLogAction = "closed by peer";
						PX_CORE_WARN("{0} hath departed", itClient->second);
					}

					// Spew something to our own log.  Note that because we put their nick
					// as the connection description, it will show up, along with their
					// transport-specific data (e.g. their IP address)
					PX_CORE_WARN("Connection {0} {1}, reason {2}: {3}",
						pInfo->m_info.m_szConnectionDescription,
						pszDebugLogAction,
						pInfo->m_info.m_eEndReason,
						pInfo->m_info.m_szEndDebug
					);

					m_mapClients.erase(itClient);

					// Send a message so everybody else knows what happened
					SendStringToAllClients(temp);
				}
				else
				{
					assert(pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
				}

				// Clean up the connection.  This is important!
				// The connection is "closed" in the network sense, but
				// it has not been destroyed.  We must close it on our end, too
				// to finish up.  The reason information do not matter in this case,
				// and we cannot linger because it's already closed on the other end,
				// so we just pass 0's.
				m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
				break;
			}

			case k_ESteamNetworkingConnectionState_Connecting:
			{
				// This must be a new connection
				assert(m_mapClients.find(pInfo->m_hConn) == m_mapClients.end());

				PX_CORE_INFO("Connection request from {0}", pInfo->m_info.m_szConnectionDescription);

				// A client is attempting to connect
				// Try to accept the connection.
				if (m_pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
				{
					// This could fail.  If the remote host tried to connect, but then
					// disconnected, the connection may already be half closed.  Just
					// destroy whatever we have on our side.
					m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
					PX_CORE_WARN("Can't accept connection.  (It was already closed?)");
					break;
				}

				// Assign the poll group
				if (!m_pInterface->SetConnectionPollGroup(pInfo->m_hConn, m_hPollGroup))
				{
					m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
					PX_CORE_WARN("Failed to set poll group?");
					break;
				}

				// Generate a random nick.  A random temporary nick
				// is really dumb and not how you would write a real chat server.
				// You would want them to have some sort of signon message,
				// and you would keep their client in a state of limbo (connected,
				// but not logged on) until them.  I'm trying to keep this example
				// code really simple.
				char nick[64];
				sprintf(nick, "BraveWarrior%d", 10000 + (rand() % 100000));

				// Send them a welcome message
				std::string wlcm = std::format("Welcome, stranger.Thou art known to us for now as '{}'; upon thine command '/nick' we shall know thee otherwise.", nick);
				SendStringToClient(pInfo->m_hConn, wlcm);
				

				// Also send them a list of everybody who is already connected
				if (m_mapClients.empty())
				{
					SendStringToClient(pInfo->m_hConn, "Thou art utterly alone.");
				}
				else
				{
					sprintf(temp, "%d companions greet you:", (int)m_mapClients.size());
					for (auto& c : m_mapClients)
						SendStringToClient(pInfo->m_hConn, std::to_string(c.second).c_str());
				}

				// Let everybody else know who they are for now
				sprintf(temp, "Hark!  A stranger hath joined this merry host.  For now we shall call them '%s'", nick);
				SendStringToAllClients(temp, pInfo->m_hConn);

				// Add them to the client list, using std::map wacky syntax
				m_mapClients[pInfo->m_hConn] = m_IDCounter++;
				break;
			}

			case k_ESteamNetworkingConnectionState_Connected:
				// We will get a callback immediately after accepting the connection.
				// Since we are the server, we can ignore this, it's not news to us.
				break;

			default:
				// Silences -Wswitch
				break;
			}
		}


		ServerInterface* ServerInterface::s_pCallbackInstance = nullptr;
		
		void ServerInterface::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
		{
			s_pCallbackInstance->OnSteamNetConnectionStatusChanged(pInfo);
		}

		
		void ServerInterface::PollConnectionStateChanges()
		{
			s_pCallbackInstance = this;
			m_pInterface->RunCallbacks();
		}

		
		inline bool ServerInterface::OnClientConnect(HSteamNetConnection client)
		{
			return false;
		}

		
		inline void ServerInterface::OnClientDisconnect(HSteamNetConnection client)
		{

		}

		
		/*void ServerInterface::OnMessage(HSteamNetConnection client, Message& msg)
		{

		}*/



		/*
		void ServerInterface::MessageClient(std::shared_ptr<Connection> client, const Message& msg)
		{
			if (client && client->IsConnected())
			{
				client->Send(msg);
			}
			else
			{
				OnClientDisconnect(client);
				client.reset();
				m_ClientMap.erase(client->GetID());
				m_DeqConnections.erase(
					std::remove(m_DeqConnections.begin(), m_DeqConnections.end(), client), m_DeqConnections.end());

			}
		}*/
	}
}