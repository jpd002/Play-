#include <string.h>
#include "alloca_def.h"
#include "CursesUtils.h"

void CursesUtils::ClearLine(WINDOW* window, int width)
{
    char* line = reinterpret_cast<char*>(alloca(width + 1));
    memset(line, ' ', width);
    line[width] = 0;
    wprintw(window, line);
}
