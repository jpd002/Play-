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

MameCompatOutput::MameCompatOutput(std::string gameId)
{
	m_doListen = true;
	Start(gameId);
}

MameCompatOutput::~MameCompatOutput()
{
	m_doListen = false;
	Stop();
	m_listenThread.join();
}

void MameCompatOutput::Listen(std::string gameId)
{
	// Initialize Winsock
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		CLog::GetInstance().Warn(LOG_NAME, "WSAStartup failed: %d\r\n", errno);
		return;
	}

	// Create socket
	m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(m_listenSocket == INVALID_SOCKET)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error creating socket: %d\r\n", errno);
		return;
	}

	// Prepare address structure
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8000);       // Port to listen on
	serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces

	// Bind socket
	if(bind(m_listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error binding socket: %d\r\n", errno);
		closesocket(m_listenSocket);
		return;
	}

	// Listen for connections
	if(listen(m_listenSocket, 5) == -1)
	{
		CLog::GetInstance().Warn(LOG_NAME, "Error listening on socket: %d\r\n", errno);
		closesocket(m_listenSocket);
		return;
	}

	CLog::GetInstance().Print(LOG_NAME, "Listening on port 8000\r\n");

	while(m_doListen)
	{
		if(!m_clientSocket)
		{
			// Accept a connection
			sockaddr_in clientAddr;
			socklen_t clientAddrSize = sizeof(clientAddr);
			int clientSocket = accept(m_listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
			if(clientSocket == -1)
			{
				CLog::GetInstance().Warn(LOG_NAME, "Error accepting connection: %d\r\n", errno);
				continue;
			}

			CLog::GetInstance().Warn(LOG_NAME, "Client connected\r\n");

			// Send hello event
			std::string hello = "mame_start = " + gameId + "\r";
			int sent = send(clientSocket, hello.c_str(), hello.length(), 0);
			if(sent == -1)
			{
				CLog::GetInstance().Warn(LOG_NAME, "Error sending hello string: %d\r\n", errno);
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
	m_listenThread = std::thread(
	    [this, gameId]() {
		    Listen(gameId);
	    });
}

void MameCompatOutput::Stop()
{
	if(m_clientSocket != INVALID_SOCKET)
	{
		// Send goodbye string
		std::string goodbye = "mame_stop = 1\r";
		int sent = send(m_clientSocket, goodbye.c_str(), goodbye.length(), 0);
		if(sent == -1)
		{
			CLog::GetInstance().Warn(LOG_NAME, "Error sending goodbye string: %d\r\n", errno);
		}
		closesocket(m_clientSocket);
		m_clientSocket = INVALID_SOCKET;
	}
	if(m_listenSocket != INVALID_SOCKET)
	{
		closesocket(m_listenSocket);
		m_listenSocket = INVALID_SOCKET;
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
			CLog::GetInstance().Warn(LOG_NAME, "Error sending recoil string: %d\r\n", errno);
			closesocket(m_clientSocket);
			m_clientSocket = 0;
		}
	}
}
#endif