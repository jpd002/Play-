#include <windows.h>
#include "PsfVm.h"
//#include <curses.h>

using namespace std;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, char* params, int showCmd)
{
    string filename = "C:\\nsf\\kh\\Kingdom Hearts (Sequences Only)\\101 - Dearly Beloved.psf2";
//    string filename = "C:\\nsf\\FF10\\111 Game Over.minipsf2";
/*
    AllocConsole();

    WINDOW *vin;
    initscr();
    start_color();
    init_pair(1,COLOR_YELLOW,COLOR_BLUE);
    init_pair(2,COLOR_BLUE,COLOR_YELLOW);
    init_pair(3,COLOR_WHITE,COLOR_YELLOW); 
    int h, w;
    getmaxyx(stdscr, h, w);
    vin=newwin(0, 0, 0, 0);
    for(unsigned int i = 0; i < 12; i++)
    {
        wmove(vin, i, 0);
        wprintw(vin, "0x00000000");
    }
    wbkgd(vin,COLOR_PAIR(3));
    wrefresh(vin);
    wgetch(vin);
    delwin(vin);
    endwin();
*/
    CPsfVm virtualMachine(filename.c_str());

    return 1;
}
