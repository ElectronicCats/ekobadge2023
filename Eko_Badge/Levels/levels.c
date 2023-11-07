#include "levels.h"

static BOOL receiveData;
static uint32_t foundationYear;
static uint16_t level;

void Levels_Init()
{
    Friends_Init();
    foundationYear = 0;
    Disable_Receive_Data();
    // Set_Level(1);
}

uint16_t Get_Level()
{
    // return level;
    return Flash_Get_Level();
}

void Set_Level(uint8_t newLevel)
{
    level = newLevel;
    Flash_Set_Level(level);
}

void Enable_Receive_Data()
{
    receiveData = TRUE;
}

void Disable_Receive_Data()
{
    receiveData = FALSE;
}

BOOL Is_Receive_Data_Enabled()
{
    return receiveData;
}

uint32_t Get_Foundation_Year()
{
    return foundationYear;
}

void Set_Foundation_Year(uint32_t year)
{
    foundationYear = year;
}
