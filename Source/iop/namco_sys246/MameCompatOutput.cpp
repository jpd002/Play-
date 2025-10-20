#ifdef _WIN32
#include <thread>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Ws2ipdef.h>
#include <sys/types.h>
#include "Log.h"
#include "MameCompatOutput.h"

#pragma comment(lib, "Ws2_32.lib") // Link with the Winsock library

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_mameCompatOutput")

void MameCompatOutput::Listen(std::string gameId)
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

			// Send hello event
			std::string hello = "mame_start = " + gameId + "\r";
			int sent = send(clientSocket, hello.c_str(), hello.length(), 0);
			if(sent == -1)
			{
				CLog::GetInstance().Warn(LOG_NAME, "Error sending hello string: %d", errno);
				closesocket(clientSocket);
				continue;
			}

			m_clientSocket = clientSocket;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void MameCompatOutput::Start(std::string gameId)
{
	MameCompatOutput network;
	std::thread t([&network](std::string gameId) {
		network.Listen(gameId);
	},
	              gameId);
	t.detach();
}

void MameCompatOutput::Stop()
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

void MameCompatOutput::SendRecoil(int value)
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