namespace Output
{
	class OutputNetwork
	{
	public:
		static void Start();
		static void Hello(std::string gameId);
		static void Stop();
		static void SendRecoil(int value);
	private:
		void Listen();
	};

	static OutputNetwork m_outputNetwork;
	static std::string m_outputRomName = "";
	static int m_clientSocket = 0;
}