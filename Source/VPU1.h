#ifndef _VPU1_H_
#define _VPU1_H_

#include "VPU.h"

class CVPU1 : public CVPU
{
public:
					CVPU1(CVIF&, unsigned int, const CVIF::VPUINIT&);
	virtual void	SaveState(Framework::CZipArchiveWriter&) override;
	virtual void	LoadState(Framework::CZipArchiveReader&) override;
	virtual uint32	GetTOP() const override;
	virtual void	Reset() override;
	virtual void	StartMicroProgram(uint32) override;

protected:
	virtual void	ExecuteCommand(StreamType&, CODE) override;
	void			Cmd_DIRECT(StreamType&, CODE);
	virtual void	Cmd_UNPACK(StreamType&, CODE, uint32) override;

private:
	uint32			m_BASE;
	uint32			m_OFST;
	uint32			m_TOP;
	uint32			m_TOPS;
};

#endif
