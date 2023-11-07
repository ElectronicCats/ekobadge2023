#ifndef FLASH_H
#define FLASH_H

#include "app_mesh_config.h"
#include "debug.h"

typedef struct {
    uint8_t address[6];
} friend_t;

/* Global define */
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;
#define PAGE_WRITE_START_ADDR  ((uint32_t)0x08068000) /* Start from 32K */
#define PAGE_WRITE_END_ADDR    ((uint32_t)0x08069000) /* End at 36K */
#define FLASH_PAGE_SIZE_LOCAL                   4096
#define FLASH_PAGES_TO_BE_PROTECTED FLASH_WRProt_Pages60to63

/* Fast Mode define */
#define FAST_FLASH_PROGRAM_START_ADDR  ((uint32_t)0x08008000)
#define FAST_FLASH_PROGRAM_END_ADDR  ((uint32_t)0x08010000)
#define FAST_FLASH_SIZE  (64*1024)

/* Reboot define */
#define REBOOT_COUNTER_ADDRESS ((uint32_t)0x0806A000)
#define REBOOT_COUNTER_ADDRESS_FLAG ((uint32_t)0x0806A002)
#define FRIENDS_COUNTER_ADDRESS ((uint32_t)0x0806A004)
#define LEVEL_ADDRESS ((uint32_t)0x0806A006)
#define FRIENDS_ADDRESS ((uint32_t)0x0806A008)

void Flash_Init();
void Flash_Erase();
uint32_t Flash_Get_Reboot_Counter();
void Flash_Test(void);
void Flash_Test_Fast(void);
void Flash_Set_Friends_Counter(uint16_t friends_counter);
uint16_t Flash_Get_Friends_Counter();
void Flash_Set_Level(uint16_t level);
uint16_t Flash_Get_Level();
void Flash_Save_Friends(friend_t *friends, uint16_t friends_counter);
void Flash_Load_Friends(friend_t *friends, uint16_t friends_counter);

#endif