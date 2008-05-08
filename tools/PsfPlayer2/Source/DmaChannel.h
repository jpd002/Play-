#ifndef _DMACHANNEL_H_
#define _DMACHANNEL_H_

namespace Psx
{
	class CDmaChannel
	{
	public:
		typedef std::tr1::function<uint32 (uint32, uint32)> ReceiveFunctionType;

								CDmaChannel(unsigned int);
		virtual					~CDmaChannel();

	private:
		ReceiveFunctionType		m_receiveFunction;
		unsigned int			m_number;
	};
};

#endif
