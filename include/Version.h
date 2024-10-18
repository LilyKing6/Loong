/* 
 * Copyright (C) 2024 Lily King
 * PROJECT:     Loong Program Language
 */


#ifndef __VERSION_H
#define __VERSION_H

#define SOFTNAME    "Loong Programming Language "

//
// Loong Version 1.0
//
#define VER_PRODUCTMAJORVERSION		1
#define VER_PRODUCTMINORVERSION		0

#define VER_PRODUCTBUILD	        35

#define VER_PRODUCTBUILD_QFE	    10

#define BUILD_LAB                  "OpenSourceBuild"

#define VER_PRODUCT2(w,x,y,z)       #w "." #x "." #y "." #z
#define VER_PRODUCT1(w,x,y,z) VER_PRODUCT2(w,x,y,z)
#define VERSION_PRODUCT VER_PRODUCT1(VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION, VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE)

#define VER_PRODUCT4(w,x)           #w "." #x
#define VER_PRODUCT3(w,x) VER_PRODUCT4(w,x)
#define KERNEL_VERSION VER_PRODUCT3(VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION)

#ifdef _WIN32
    #ifdef _WIN64
        #define _PLATFORM "WIN64"
    #else
        #define _PLATFORM "WIN32"
    #endif
#else
    #ifdef __linux__
        #ifdef __x86_64__
            #define _PLATFORM "LINUX64"
        #elif __i386__
            #define _PLATFORM "LINUX32"
        #endif
    #else
        #ifdef __x86_64__
            #define _PLATFORM "MAC64"
        #elif __i386__
            #define _PLATFORM "MAC32"
        #endif
    #endif
#endif

#endif // __VERSION_H

