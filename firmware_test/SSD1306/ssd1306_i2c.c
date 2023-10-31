#include "ssd1306_i2c.h"
#include "ch32v003fun.h"

/*
 * init just I2C
 */
void ssd1306_i2c_setup(void)
{
	uint16_t tempreg;
	
	// Reset I2C2 to init all regs
	RCC->APB1PRSTR |= RCC_APB1Periph_I2C2;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C2;
	
	// set freq
	tempreg = I2C2->CTLR2;
	tempreg &= ~I2C_CTLR2_FREQ;
	tempreg |= (FUNCONF_SYSTEM_CORE_CLOCK/SSD1306_I2C_PRERATE)&I2C_CTLR2_FREQ;
	I2C2->CTLR2 = tempreg;
	
	// Set clock config
	tempreg = 0;
#if (SSD1306_I2C_CLKRATE <= 100000)
	// standard mode good to 100kHz
	tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(2*SSD1306_I2C_CLKRATE))&SSD1306_I2C_CKCFGR_CCR;
#else
	// fast mode over 100kHz
#ifndef SSD1306_I2C_DUTY
	// 33% duty cycle
	tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(3*SSD1306_I2C_CLKRATE))&SSD1306_I2C_CKCFGR_CCR;
#else
	// 36% duty cycle
	tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(25*SSD1306_I2C_CLKRATE))&I2C_CKCFGR_CCR;
	tempreg |= I2C_CKCFGR_DUTY;
#endif
	tempreg |= I2C_CKCFGR_FS;
#endif
	I2C2->CKCFGR = tempreg;

#ifdef SSD1306_I2C_IRQ
	// enable IRQ driven operation
	NVIC_EnableIRQ(I2C2_EV_IRQn);
	
	// initialize the state
	ssd1306_i2c_irq_state = 0;
#endif
	
	// Enable I2C
	I2C2->CTLR1 |= I2C_CTLR1_PE;

	// set ACK mode
	I2C2->CTLR1 |= I2C_CTLR1_ACK;
}

/*
 * error descriptions
 */
char *errstr[] =
{
	"not busy",
	"master mode",
	"transmit mode",
	"tx empty",
	"transmit complete",
};

/*
 * error handler
 */
uint8_t ssd1306_i2c_error(uint8_t err)
{
	// report error
	printf("ssd1306_i2c_error - timeout waiting for %s\n\r", errstr[err]);
	
	// reset & initialize I2C
	ssd1306_i2c_setup();

	return 1;
}

// event codes we use
#define  SSD1306_I2C_EVENT_MASTER_MODE_SELECT ((uint32_t)0x00030001)  /* BUSY, MSL and SB flag */
#define  SSD1306_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ((uint32_t)0x00070082)  /* BUSY, MSL, ADDR, TXE and TRA flags */
#define  SSD1306_I2C_EVENT_MASTER_BYTE_TRANSMITTED ((uint32_t)0x00070084)  /* TRA, BUSY, MSL, TXE and BTF flags */

/*
 * check for 32-bit event codes
 */
uint8_t ssd1306_i2c_chk_evt(uint32_t event_mask)
{
	/* read order matters here! STAR1 before STAR2!! */
	uint32_t status = I2C2->STAR1 | (I2C2->STAR2<<16);
	return (status & event_mask) == event_mask;
}

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
uint8_t ssd1306_i2c_send(uint8_t addr, uint8_t *data, uint8_t sz)
{
	int32_t timeout;
	
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

	// send data one byte at a time
	while(sz--)
	{
		// wait for TX Empty
		timeout = TIMEOUT_MAX;
		while(!(I2C2->STAR1 & I2C_STAR1_TXE) && (timeout--));
		if(timeout==-1)
			return ssd1306_i2c_error(3);
		
		// send command
		I2C2->DATAR = *data++;
	}

	// wait for tx complete
	timeout = TIMEOUT_MAX;
	while((!ssd1306_i2c_chk_evt(SSD1306_I2C_EVENT_MASTER_BYTE_TRANSMITTED)) && (timeout--));
	if(timeout==-1) {
		printf("Here\n");
		return ssd1306_i2c_error(4);
	}

	// set STOP condition
	I2C2->CTLR1 |= I2C_CTLR1_STOP;
	
	// we're happy
	return 0;
}
#endif

/*
 * high-level packet send for I2C
 */
uint8_t ssd1306_pkt_send(uint8_t *data, uint8_t sz, uint8_t cmd)
{
	uint8_t pkt[33];
	
	/* build command or data packets */
	if(cmd)
	{
		pkt[0] = 0;
		pkt[1] = *data;
	}
	else
	{
		pkt[0] = 0x40;
		memcpy(&pkt[1], data, sz);
	}
	return ssd1306_i2c_send(SSD1306_I2C_ADDR, pkt, sz+1);
}

/*
 * reset is not used for SSD1306 I2C interface
 */
void ssd1306_rst(void)
{
}