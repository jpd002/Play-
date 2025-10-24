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
			int m_clientSocket = 0;
			std::thread m_listenThread;
		};
	}
}