#ifndef _DEBUGVIEW_H_
#define _DEBUGVIEW_H_

#include <curses.h>

class CDebugView
{
public:
    virtual ~CDebugView() {}
    virtual void Update(WINDOW*, int, int) = 0;

private:

};


#endif
