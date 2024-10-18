/*
License for Loong

Copyright 2024 Lily King

All Rights Reserved

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE. 
*/

#include "GlobalData.h"

LGlobalData::LGlobalData()
{
}

LGlobalData::~LGlobalData()
{
    clear_all_nodes();
    clear_globals(); 
}

void LGlobalData::clear_all_nodes()
{
    for (size_t i = 0; i < m_vecNodes.size(); i++)
        delete m_vecNodes[i]; 
    m_vecNodes.clear(); 
}

void LGlobalData::clear_globals()
{
    m_functions.clear();
    m_globals.clear(); 
}

void LCheckStack::push()
{
    LGlobalCheck check;
    m_stack.push_back(check);
}

void LCheckStack::pop()
{
    if (m_stack.size() == 0)
        return;
    m_stack.pop_back();
}

LGlobalCheck& LCheckStack::top()
{
    static LGlobalCheck emptyRes;
    if (m_stack.size() == 0)
        return emptyRes;

    return m_stack[m_stack.size() - 1];
}

void LCheckStack::add_node(AST* node)
{
    if (m_stack.size() == 0) return; 
    m_stack[m_stack.size() - 1].nodes().push_back(node);
}

void LCheckStack::add_param(string name)
{
    if (m_stack.size() == 0) return; 
    m_stack[m_stack.size() - 1].params()[name] = true;
}

void LCheckStack::add_assign(string name)
{
    if (m_stack.size() == 0) return;
    m_stack[m_stack.size() - 1].assigns()[name] = true;
}
