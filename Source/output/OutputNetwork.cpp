#ifdef _WIN32
#include <thread>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Ws2ipdef.h>
#include <sys/types.h>
#include "Log.h"
#include "output/OutputNetwork.h"

#pragma comment(lib, "Ws2_32.lib") // Link with the Winsock library

using namespace Output;

#define LOG_NAME ("output_network")

void OutputNetwork::Listen()
{
	// Initialize Winsock
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		CLog::GetInstance().Warn(LOG_NAME, "WSAStartup failed: %d\n", errno);
		return;
	}

	// Create socket
	int listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listenSocket == -1)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error creating socket: %d", errno);
		return;
	}

	// Prepare address structure
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8000);       // Port to listen on
	serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces

	// Bind socket
	if(bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error binding socket: %d", errno);
		closesocket(listenSocket);
		return;
	}

	// Listen for connections
	if(listen(listenSocket, 5) == -1)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error listening on socket: %d", errno);
		closesocket(listenSocket);
		return;
	}

	CLog::GetInstance().Print(LOG_NAME, "Listening on port 8000");

	while(true)
	{
		if(!m_clientSocket)
		{
			// Accept a connection
			sockaddr_in clientAddr;
			socklen_t clientAddrSize = sizeof(clientAddr);
			int clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
			if(clientSocket == -1)
			{
				CLog::GetInstance().Warn(LOG_NAME, "Error accepting connection: %d", errno);
				continue;
			}

			CLog::GetInstance().Warn(LOG_NAME, "Client connected");

			if(Output::m_outputRomName.compare("") != 0)
			{
				// Send hello event
				std::string hello = "mame_start = " + Output::m_outputRomName + "\r";
				int sent = send(clientSocket, hello.c_str(), hello.length(), 0);
				if(sent == -1)
				{
					CLog::GetInstance().Warn(LOG_NAME, "Error sending hello string: %d", errno);
					closesocket(clientSocket);
					continue;
				}
			}

			m_clientSocket = clientSocket;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void OutputNetwork::Start()
{
	OutputNetwork network;
	std::thread t([&network]() {
		network.Listen();
	});
	m_outputNetwork = network;
	t.detach();
}

void OutputNetwork::Stop()
{
	if(m_clientSocket)
	{
		// Send goodbye string
		std::string goodbye = "mame_stop = 1\r";
		int sent = send(m_clientSocket, goodbye.c_str(), goodbye.length(), 0);
		if(sent == -1)
		{
			CLog::GetInstance().Warn(LOG_NAME, "Error sending goodbye string: %d", errno);
			closesocket(m_clientSocket);
			m_clientSocket = 0;
		}
	}
	WSACleanup();
}

void OutputNetwork::Hello(std::string gameId)
{
	Output::m_outputRomName = gameId;

	if(m_clientSocket)
	{
		// Send hello event
		std::string hello = "mame_start = " + gameId + "\r";
		int sent = send(m_clientSocket, hello.c_str(), hello.length(), 0);
		if(sent == -1)
		{
			CLog::GetInstance().Warn(LOG_NAME, "Error sending hello string: %d", errno);
			closesocket(m_clientSocket);
			m_clientSocket = 0;
		}
	}
}

void OutputNetwork::SendRecoil(int value)
{
	if(m_clientSocket)
	{
		// Send recoil event
		std::string recoil = "mcuout1 = " + std::to_string(value) + "\r";
		int sent = send(m_clientSocket, recoil.c_str(), recoil.length(), 0);
		if(sent == -1)
		{
			CLog::GetInstance().Warn(LOG_NAME, "Error sending recoil string: %d", errno);
			closesocket(m_clientSocket);
			m_clientSocket = 0;
		}
	}
}
#endif