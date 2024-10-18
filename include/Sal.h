#ifndef SAL_H
#define SAL_H

#include <iostream>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

enum Color 
{
    BLACK,           // 黑色        0
    RED,             // 红色        1
    GREEN,           // 绿色        2
    YELLOW,          // 黄色        3
    BLUE,            // 蓝色        4
    MAGENTA,         // 品红色      5
    CYAN,            // 青色        6
    WHITE,           // 白色        7
    GRAY,            // 灰色        8
    BRIGHT_RED,      // 亮红色      9
    BRIGHT_GREEN,    // 亮绿色      10
    BRIGHT_YELLOW,   // 亮黄色      11
    BRIGHT_BLUE,     // 亮蓝色      12
    BRIGHT_MAGENTA,  // 亮品红色    13
    BRIGHT_CYAN,     // 亮青色      14
    BRIGHT_WHITE     // 亮白色      15
};
void PrintColor(const std::string& text, Color color);

#endif /* SAL_H */
