#ifndef _GIF_H_
#define _GIF_H_

#include "Types.h"
#include "GSHandler.h"
#include <boost/thread.hpp>

class CGIF
{
public:
                    CGIF(CGSHandler*&, uint8*, uint8*);
    virtual         ~CGIF();

    void			Reset();
    uint32          ReceiveDMA(uint32, uint32, uint32, bool);
    uint32          ProcessPacket(uint8*, uint32, uint32);

private:
    uint32			ProcessPacked(CGSHandler::RegisterWriteList&, uint8*, uint32, uint32);
    uint32			ProcessRegList(CGSHandler::RegisterWriteList&, uint8*, uint32, uint32);
    uint32			ProcessImage(uint8*, uint32, uint32);

    uint16			m_nLoops;
    uint8			m_nCmd;
    uint8			m_nRegs;
    uint8			m_nRegsTemp;
    uint64			m_nRegList;
    bool			m_nEOP;
    uint32			m_nQTemp;
    uint8*          m_ram;
    uint8*          m_spr;
    CGSHandler*&    m_gs;
    boost::mutex    m_pathMutex;
};

#endif
