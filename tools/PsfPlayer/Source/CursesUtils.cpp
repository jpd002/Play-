#include <malloc.h>
#include <string.h>
#include "CursesUtils.h"

void CursesUtils::ClearLine(WINDOW* window, int width)
{
    char* line = reinterpret_cast<char*>(_alloca(width + 1));
    memset(line, ' ', width);
    line[width] = 0;
    wprintw(window, line);
}
