#include "MemoryView.h"
#include "CursesUtils.h"

CMemoryView::CMemoryView(CMIPS& cpu) :
m_cpu(cpu),
m_viewAddress(0)
{

}

CMemoryView::~CMemoryView()
{

}

void CMemoryView::Update(WINDOW* window, int width, int height)
{
    const int bytesPerLine = 16;
    uint32 address = m_viewAddress;

    //Draw the ruler
    {
        wmove(window, 0, 0);
        CursesUtils::ClearLine(window, width);

        unsigned int baseX = 9;
        wmove(window, 0, baseX);

        for(unsigned int i = 0; i < bytesPerLine; i++)
        {
            wprintw(window, "%0.2X ", i);
        }

        for(unsigned int i = 0; i < bytesPerLine; i++)
        {
            wprintw(window, "%X", i);
        }
    }

    for(int i = 1; i < height; i++)
    {
        wmove(window, i, 0);
        CursesUtils::ClearLine(window, width);

        wmove(window, i, 0);
        wprintw(window, "%0.8X ", address);

        for(unsigned int j = 0; j < bytesPerLine; j++)
        {
            uint8 value = m_cpu.m_pMemoryMap->GetByte(address + j);
            wprintw(window, "%0.2X ", value);
        }
        
        for(unsigned int j = 0; j < bytesPerLine; j++)
        {
            uint8 value = m_cpu.m_pMemoryMap->GetByte(address + j);
            if(!isprint(value))
            {
                value = '.';
            }
            wprintw(window, "%c", value);
        }

        address += bytesPerLine;
    }   
}

void CMemoryView::SetViewAddress(uint32 viewAddress)
{
    m_viewAddress = viewAddress & ~0xF;
}
