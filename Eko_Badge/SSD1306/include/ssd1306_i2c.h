/*
 * Single-File-Header for SSD1306 I2C interface
 * 05-07-2023 E. Brombaugh
 */

#ifndef _SSD1306_I2C_H
#define _SSD1306_I2C_H

#include <string.h>
#include "debug.h"
#include "ch32v003fun.h"

// SSD1306 I2C address
#define SSD1306_I2C_ADDR 0x3c

// I2C Bus clock rate - must be lower the Logic clock rate
#define SSD1306_I2C_CLKRATE 1000000

// I2C Logic clock rate - must be higher than Bus clock rate
#define SSD1306_I2C_PRERATE 2000000

// uncomment this for high-speed 36% duty cycle, otherwise 33%
#define SSD1306_I2C_DUTY

// I2C Timeout count
#define TIMEOUT_MAX 100000

// uncomment this to enable IRQ-driven operation
//#define SSD1306_I2C_IRQ

#ifdef SSD1306_I2C_IRQ
// some stuff that IRQ mode needs
volatile uint8_t ssd1306_i2c_send_buffer[64], *ssd1306_i2c_send_ptr, ssd1306_i2c_send_sz, ssd1306_i2c_irq_state;

// uncomment this to enable time diags in IRQ
//#define IRQ_DIAG
#endif

/*
 * init just I2C
 */
void ssd1306_i2c_setup(void);

/*
 * error descriptions
 */
extern char *errstr[];

/*
 * error handler
 */
uint8_t ssd1306_i2c_error(uint8_t err);

// event codes we use
#define  SSD1306_I2C_EVENT_MASTER_MODE_SELECT ((uint32_t)0x00030001)  /* BUSY, MSL and SB flag */
#define  SSD1306_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ((uint32_t)0x00070082)  /* BUSY, MSL, ADDR, TXE and TRA flags */
#define  SSD1306_I2C_EVENT_MASTER_BYTE_TRANSMITTED ((uint32_t)0x00070084)  /* TRA, BUSY, MSL, TXE and BTF flags */

/*
 * check for 32-bit event codes
 */
uint8_t ssd1306_i2c_chk_evt(uint32_t event_mask);

#ifdef SSD1306_I2C_IRQ
/*
 * packet send for IRQ-driven operation
 */
uint8_t ssd1306_i2c_send(uint8_t addr, uint8_t *data, uint8_t sz)
{
	int32_t timeout;
	
#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(3));
#endif
	
	// error out if buffer under/overflow
	if((sz > sizeof(ssd1306_i2c_send_buffer)) || !sz)
		return 2;
	
	// wait for previous packet to finish
	while(ssd1306_i2c_irq_state);
	
#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(16+3));
	GPIOC->BSHR = (1<<(4));
#endif
	
	// init buffer for sending
	ssd1306_i2c_send_sz = sz;
	ssd1306_i2c_send_ptr = ssd1306_i2c_send_buffer;
	memcpy((uint8_t *)ssd1306_i2c_send_buffer, data, sz);
	
	// wait for not busy
	timeout = TIMEOUT_MAX;
	while((I2C2->STAR2 & I2C_STAR2_BUSY) && (timeout--));
	if(timeout==-1)
		return ssd1306_i2c_error(0);

	// Set START condition
	I2C2->CTLR1 |= I2C_CTLR1_START;

	// wait for master mode select
	timeout = TIMEOUT_MAX;
	while((!ssd1306_i2c_chk_evt(SSD1306_I2C_EVENT_MASTER_MODE_SELECT)) && (timeout--));
	if(timeout==-1)
		return ssd1306_i2c_error(1);
	
	// send 7-bit address + write flag
	I2C2->DATAR = addr<<1;

	// wait for transmit condition
	timeout = TIMEOUT_MAX;
	while((!ssd1306_i2c_chk_evt(SSD1306_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) && (timeout--));
	if(timeout==-1)
		return ssd1306_i2c_error(2);

	// Enable TXE interrupt
	I2C2->CTLR2 |= I2C_CTLR2_ITBUFEN | I2C_CTLR2_ITEVTEN;
	ssd1306_i2c_irq_state = 1;

#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(16+4));
#endif
	
	// exit
	return 0;
}

/*
 * IRQ handler for I2C events
 */
void I2C2_EV_IRQHandler(void) __attribute__((interrupt));
void I2C2_EV_IRQHandler(void)
{
	uint16_t STAR1, STAR2 __attribute__((unused));
	
#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(4));
#endif

	// read status, clear any events
	STAR1 = I2C2->STAR1;
	STAR2 = I2C2->STAR2;
	
	/* check for TXE */
	if(STAR1 & I2C_STAR1_TXE)
	{
		/* check for remaining data */
		if(ssd1306_i2c_send_sz--)
			I2C2->DATAR = *ssd1306_i2c_send_ptr++;

		/* was that the last byte? */
		if(!ssd1306_i2c_send_sz)
		{
			// disable TXE interrupt
			I2C2->CTLR2 &= ~(I2C_CTLR2_ITBUFEN | I2C_CTLR2_ITEVTEN);
			
			// reset IRQ state
			ssd1306_i2c_irq_state = 0;
			
			// wait for tx complete
			while(!ssd1306_i2c_chk_evt(SSD1306_I2C_EVENT_MASTER_BYTE_TRANSMITTED));

			// set STOP condition
			I2C2->CTLR1 |= I2C_CTLR1_STOP;
		}
	}

#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(16+4));
#endif
}
#else
/*
 * low-level packet send for blocking polled operation via i2c
 */
uint8_t ssd1306_i2c_send(uint8_t addr, uint8_t *data, uint8_t sz);
#endif

/*
 * high-level packet send for I2C
 */
uint8_t ssd1306_pkt_send(uint8_t *data, uint8_t sz, uint8_t cmd);

/*
 * reset is not used for SSD1306 I2C interface
 */
void ssd1306_rst(void);

#endif
