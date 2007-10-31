#include "TextDebugger.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include "CursesUtils.h"

using namespace std;
using namespace boost;

CTextDebugger::CTextDebugger(CMIPS& cpu) :
m_cpu(cpu),
//m_window(NULL),
m_commandWindow(NULL),
m_primaryWindow(NULL),
m_height(0),
m_width(0),
m_disAsmView(cpu),
m_memoryView(cpu),
m_registerView(cpu),
m_currentView(&m_disAsmView)
{

}

CTextDebugger::~CTextDebugger()
{

}

void CTextDebugger::Run()
{
    WINDOW* _stdscr = initscr();
    cbreak();
    noecho();

    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLUE);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_WHITE); 

    getmaxyx(_stdscr, m_height, m_width);

//    m_window = newwin(0, 0, 0, 0);
//    getmaxyx(m_window, m_height, m_width);

    const int regViewHeight = 7;
    const int commandViewHeight = 1;
    const int primaryViewHeight = m_height - regViewHeight - commandViewHeight;

    m_commandWindow = newwin(commandViewHeight, m_width, m_height - commandViewHeight, 0);
    m_primaryWindow = newwin(primaryViewHeight, m_width, 0, 0);
    m_secondaryWindow = newwin(regViewHeight, m_width, primaryViewHeight, 0);

    keypad(m_commandWindow, true);
    wbkgd(m_commandWindow, COLOR_PAIR(2));

//    uint32 address = cpu.m_State.nPC;
//    address = 0x1005E0;
//    UpdateDisassembly(vin, cpu, address, w, h - 1);
//    UpdateMemory(vin, cpu, address, w, h - 1);

    m_disAsmView.SetViewAddress(m_cpu.m_State.nPC);

    string command;
    while(1)
    {
        if(m_currentView != NULL)
        {
            m_currentView->Update(m_primaryWindow, m_width, m_height - 1);
        }
        m_registerView.Update(m_secondaryWindow, m_width, m_height);

        wmove(m_commandWindow, 0, 0);
        CursesUtils::ClearLine(m_commandWindow, m_width);
        wmove(m_commandWindow, 0, 0);
        wprintw(m_commandWindow, command.c_str());
        wrefresh(m_commandWindow);
        int ch = wgetch(m_commandWindow);

        if(ch == KEY_F(10))
        {
            m_cpu.Step();
        }
        else if(ch == 8)
        {
            if(command.length() != 0)
            {
                command.erase(command.end() - 1);
            }
        }
        else if(ch == 10)
        {
            ExecuteCommand(command);
            command = "";
        }
        else if((ch < 0x100) && (isalpha(ch) || isdigit(ch) || isspace(ch)))
        {
            command += static_cast<char>(ch);
        }
    }

    delwin(m_commandWindow);
    delwin(m_primaryWindow);
//    delwin(m_window);
    endwin();

    m_commandWindow = NULL;
    m_primaryWindow = NULL;
//    m_window = NULL;
}

void CTextDebugger::ExecuteCommand(const string& command)
{
    typedef vector<string> SplitVectorType;
    SplitVectorType tokens;
    split(tokens, command, is_any_of(" "));
    if(tokens.size() > 0)
    {
        const string& commandName = tokens[0];
        if(!commandName.compare("m"))
        {
            m_currentView = &m_memoryView;
            if(tokens.size() > 1)
            {
                const string& addressString = tokens[1];
                uint32 address;
                if(sscanf(addressString.c_str(), "%x", &address))
                {
                    m_memoryView.SetViewAddress(address);
                }
            }
        }
        else if(!commandName.compare("d"))
        {
            m_currentView = &m_disAsmView;
            if(tokens.size() > 1)
            {
                const string& addressString = tokens[1];
                uint32 address;
                if(sscanf(addressString.c_str(), "%x", &address))
                {
                    m_disAsmView.SetViewAddress(address);
                }
                if(!addressString.compare("pc"))
                {
                    m_disAsmView.SetViewAddress(m_cpu.m_State.nPC);
                }
            }
        }
    }
}
