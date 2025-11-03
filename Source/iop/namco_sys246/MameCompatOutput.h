#pragma once

#include "SocketDef.h"

namespace Iop
{
	namespace Namco
	{
		class MameCompatOutput
		{
		public:
			MameCompatOutput(std::string gameId);
			~MameCompatOutput();
			void SendRecoil(int value);

		private:
			void Start(std::string gameId);
			void Listen(std::string gameId);
			void Stop();

			bool m_doListen = false;
			SOCKET m_listenSocket = INVALID_SOCKET;
			SOCKET m_clientSocket = INVALID_SOCKET;
			std::thread m_listenThread;
		};
	}
}