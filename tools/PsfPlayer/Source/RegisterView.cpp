#include "RegisterView.h"
#include "CursesUtils.h"

CRegisterView::CRegisterView(CMIPS& cpu) :
m_cpu(cpu)
{
    
}

CRegisterView::~CRegisterView()
{

}

void CRegisterView::Update(WINDOW* window, int width, int height)
{
    getmaxyx(window, width, height);

    const unsigned int registerCount = 32;
    const unsigned int columnCount = 5;
    const unsigned int lineCount = (registerCount + (columnCount - 1)) / (columnCount);

    int reg = 0;

    for(unsigned int i = 0; i < lineCount && reg < registerCount; i++)
    {
        wmove(window, i, 0);
        CursesUtils::ClearLine(window, width);
        wmove(window, i, 0);
        for(unsigned int j = 0; j < columnCount && reg < registerCount; j++)
        {
            const char* regName = CMIPS::m_sGPRName[reg];
            uint32 regValue = m_cpu.m_State.nGPR[reg].nV0;
            wprintw(window, "%s: 0x%0.8X  ", regName, regValue);
            reg++;
        }
    }

    wrefresh(window);
}

