#pragma once

#include "Vif.h"

class CGIF;

class CVif1 : public CVif
{
public:
					CVif1(unsigned int, CVpu&, CGIF&, uint8*, uint8*);
	virtual			~CVif1();

	virtual void	Reset() override;
	virtual void	SaveState(Framework::CZipArchiveWriter&) override;
	virtual void	LoadState(Framework::CZipArchiveReader&) override;

	virtual uint32	GetTOP() const override;

private:
	typedef std::vector<uint8> ByteArray;

	virtual void	ExecuteCommand(StreamType&, CODE) override;

	void			Cmd_DIRECT(StreamType&, CODE);
	virtual void	Cmd_UNPACK(StreamType&, CODE, uint32) override;

	virtual void	PrepareMicroProgram() override;

	CGIF&			m_gif;

	uint32			m_BASE;
	uint32			m_OFST;
	uint32			m_TOP;
	uint32			m_TOPS;

	ByteArray		m_directBuffer;
};
