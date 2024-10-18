#include <iostream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

#include "Sal.h"

void PrintColor(const std::string& text, Color color) 
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD originalColorAttrs = 0;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        originalColorAttrs = csbi.wAttributes;
    }

    WORD colorCode;
    switch (color) {
        case BLACK:         colorCode = 0; break;
        case RED:           colorCode = FOREGROUND_RED; break;
        case GREEN:         colorCode = FOREGROUND_GREEN; break;
        case YELLOW:        colorCode = FOREGROUND_RED | FOREGROUND_GREEN; break;
        case BLUE:          colorCode = FOREGROUND_BLUE; break;
        case MAGENTA:       colorCode = FOREGROUND_RED | FOREGROUND_BLUE; break;
        case CYAN:          colorCode = FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case WHITE:         colorCode = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case GRAY:          colorCode = FOREGROUND_INTENSITY; break;
        case BRIGHT_RED:    colorCode = FOREGROUND_INTENSITY | FOREGROUND_RED; break;
        case BRIGHT_GREEN:  colorCode = FOREGROUND_INTENSITY | FOREGROUND_GREEN; break;
        case BRIGHT_YELLOW: colorCode = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN; break;
        case BRIGHT_BLUE:   colorCode = FOREGROUND_INTENSITY | FOREGROUND_BLUE; break;
        case BRIGHT_MAGENTA:colorCode = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE; break;
        case BRIGHT_CYAN:   colorCode = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        case BRIGHT_WHITE:  colorCode = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        default:            colorCode = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }
    SetConsoleTextAttribute(hConsole, colorCode);
    std::cout << text;
    SetConsoleTextAttribute(hConsole, originalColorAttrs);
#else
    int colorCode;
    switch (color) {
        case BLACK:         colorCode = 30; break;
        case RED:           colorCode = 31; break;
        case GREEN:         colorCode = 32; break;
        case YELLOW:        colorCode = 33; break;
        case BLUE:          colorCode = 34; break;
        case MAGENTA:       colorCode = 35; break;
        case CYAN:          colorCode = 36; break;
        case WHITE:         colorCode = 37; break;
        case GRAY:          colorCode = 90; break;
        case BRIGHT_RED:    colorCode = 91; break;
        case BRIGHT_GREEN:  colorCode = 92; break;
        case BRIGHT_YELLOW: colorCode = 93; break;
        case BRIGHT_BLUE:   colorCode = 94; break;
        case BRIGHT_MAGENTA:colorCode = 95; break;
        case BRIGHT_CYAN:   colorCode = 96; break;
        case BRIGHT_WHITE:  colorCode = 97; break;
        default:            colorCode = 37; break;
    }
    std::cout << "\033[" << colorCode << "m" << text << "\033[0m";
#endif
}
