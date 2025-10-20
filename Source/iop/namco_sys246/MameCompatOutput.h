namespace Iop
{
	namespace Namco
	{
		class MameCompatOutput
		{
		public:
			static void Start(std::string gameId);
			static void Stop();
			static void SendRecoil(int value);

		private:
			void Listen(std::string gameId);
		};

		static int m_clientSocket = 0;
	}
}