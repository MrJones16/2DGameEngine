#pragma once

#include "NetworkCommon.h"
#include "NetworkMessage.h"
#include "NetworkThreadSafeQueue.h"
#include "NetworkConnection.h"

namespace Pyxis
{
	namespace Network
	{
		template<typename T>
		class ClientInterface
		{
		public:
			ClientInterface() : m_Socket(m_Context)
			{
				//initialize the socket with the io context
			}

			virtual ~ClientInterface()
			{
				//if the client is destroyed, always try and disconnect
				Disconnect();
			}
		public:
			// Connect to server with hostname/ip-address and port
			inline bool Connect(const std::string& host, const uint16_t port)
			{
				try
				{
					//resolve hostname/ip address into tangible physical address
					asio::ip::tcp::resolver resolver(m_Context);
					asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

					//create connection
					m_Connection = std::make_unique<Connection<T>>(
						Connection<T>::Owner::client,
						m_Context, 
						asio::ip::tcp::socket(m_Context), 
						m_QueueMessagesIn);

					//tell the connection object to connect to server
					m_Connection->ConnectToServer(endpoints);

					//start context thread
					m_ContextThread = std::thread([this]() {m_Context.run(); });
				}
				catch (std::exception& e)
				{
					PX_CORE_ERROR("Client Exception: {0}", e.what());
					return false;
				}
				return true;
			}

			// Disconnect from server
			inline void Disconnect()
			{
				//if connection exists and it's connected
				if (IsConnected())
				{
					//disconnect from server gracefully
					m_Connection->Disconnect();
				}

				//either way. we're also done with the asio context
				m_Context.stop();

				//and its thread
				if (m_ContextThread.joinable())
					m_ContextThread.join();

				//destroy the connection object
				m_Connection.release();
			}

			// Check if client is actually connected to a server
			inline bool IsConnected()
			{
				if (m_Connection)
					return m_Connection->IsConnected();
				else
					return false;
			}

			// Retrieve queue of messages from server
			ThreadSafeQueue<OwnedMessage<T>>& Incoming()
			{
				return m_QueueMessagesIn;
			}

			//Send a message to the server
			void Send(const Message<T>& msg)
			{
				m_Connection->Send(msg);
			}


		protected:
			//asio context handles the data transfer
			asio::io_context m_Context;

			//but needs a thread of its own to execute its work commands
			std::thread m_ContextThread;

			//this is the hardware socket that is connected to the server
			asio::ip::tcp::socket m_Socket;

			//the client has a single instance of a "connection" object, which handles data transfer
			std::unique_ptr<Connection<T>> m_Connection;

		private:
			//this is the thread safe queue of incoming messages from the server
			ThreadSafeQueue<OwnedMessage<T>> m_QueueMessagesIn;
		};
	}
}