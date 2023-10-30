// This contains a copy of ch32v00x.h and core_riscv.h ch32v00x_conf.h and other misc functions  See copyright notice at end.

#ifndef __CH32V00x_H
#define __CH32V00x_H

#include "funconfig.h"
#include "debug.h"

/*****************************************************************************
	CH32V003 Fun Configs:

#define FUNCONF_USE_PLL 1               // Use built-in 2x PLL 
#define FUNCONF_USE_HSI 1               // Use HSI Internal Oscillator
#define FUNCONF_USE_HSE 0               // Use External Oscillator
#define FUNCONF_HSITRIM 0x10            // Use factory calibration on HSI Trim.
#define FUNCONF_SYSTEM_CORE_CLOCK  48000000  // Computed Clock in Hz.
#define FUNCONF_HSE_BYPASS 0            // Use HSE Bypass feature (for oscillator input)
#define FUNCONF_USE_CLK_SEC	1			// Use clock security system, enabled by default
#define FUNCONF_USE_DEBUGPRINTF 1
#define FUNCONF_USE_UARTPRINTF  0
#define FUNCONF_NULL_PRINTF 0           // Have printf but direct it "nowhere"
#define FUNCONF_SYSTICK_USE_HCLK 0      // Should systick be at 48 MHz or 6MHz?
#define FUNCONF_TINYVECTOR 0            // If enabled, Does not allow normal interrupts.
#define FUNCONF_UART_PRINTF_BAUD 115200 // Only used if FUNCONF_USE_UARTPRINTF is set.
#define FUNCONF_DEBUGPRINTF_TIMEOUT 160000 // Arbitrary time units
*/

#if !defined(FUNCONF_USE_DEBUGPRINTF) && !defined(FUNCONF_USE_UARTPRINTF)
	#define FUNCONF_USE_DEBUGPRINTF 1
#endif

#if defined(FUNCONF_USE_UARTPRINTF) && FUNCONF_USE_UARTPRINTF && !defined(FUNCONF_UART_PRINTF_BAUD)
	#define FUNCONF_UART_PRINTF_BAUD 115200
#endif

#if defined(FUNCONF_USE_DEBUGPRINTF) && FUNCONF_USE_DEBUGPRINTF && !defined(FUNCONF_DEBUGPRINTF_TIMEOUT)
	#define FUNCONF_DEBUGPRINTF_TIMEOUT 160000
#endif

#if defined(FUNCONF_USE_HSI) && defined(FUNCONF_USE_HSE) && FUNCONF_USE_HSI && FUNCONF_USE_HSE
       #error FUNCONF_USE_HSI and FUNCONF_USE_HSE cannot both be set
#endif

#if !defined( FUNCONF_USE_HSI ) && !defined( FUNCONF_USE_HSE )
	#define FUNCONF_USE_HSI 1 // Default to use HSI
	#define FUNCONF_USE_HSE 0
#endif

#if !defined( FUNCONF_USE_PLL )
	#define FUNCONF_USE_PLL 1 // Default to use PLL
#endif

#if !defined( FUNCONF_USE_CLK_SEC )
	#define FUNCONF_USE_CLK_SEC  1// use clock security system by default
#endif	

// #ifndef HSE_VALUE
// 	#define HSE_VALUE                 (24000000) // Value of the External oscillator in Hz, default
// #endif

// #ifndef HSI_VALUE
// 	#define HSI_VALUE                 (24000000) // Value of the Internal oscillator in Hz, default.
// #endif

#ifndef FUNCONF_HSITRIM
	#define FUNCONF_HSITRIM 0x10  // Default (Chip default)
#endif

#ifndef FUNCONF_USE_PLL
	#define FUNCONF_USE_PLL 1     // Default, Use PLL.
#endif

#if !defined( FUNCONF_PLL_MULTIPLIER )
	#if defined(FUNCONF_USE_PLL) && FUNCONF_USE_PLL
		#define FUNCONF_PLL_MULTIPLIER 2
	#else
		#define FUNCONF_PLL_MULTIPLIER 1
	#endif
#endif

#ifndef FUNCONF_SYSTEM_CORE_CLOCK
	#if defined(FUNCONF_USE_HSI) && FUNCONF_USE_HSI
		#define FUNCONF_SYSTEM_CORE_CLOCK ((HSI_VALUE)*(FUNCONF_PLL_MULTIPLIER))
	#elif defined(FUNCONF_USE_HSE) && FUNCONF_USE_HSE
		#define FUNCONF_SYSTEM_CORE_CLOCK ((HSE_VALUE)*(FUNCONF_PLL_MULTIPLIER))
	#else
		#error Must define either FUNCONF_USE_HSI or FUNCONF_USE_HSE to be 1.
	#endif
#endif

#endif
