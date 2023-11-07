#include "friend.h"

BOOL enableFriendSearch;
uint16_t friendsCounter;
friend_t friends[FRIENDS_MAX];

void Friends_Init()
{
    enableFriendSearch = FALSE;
    // Flash_Set_Friends_Counter(29);
    friendsCounter = Flash_Get_Friends_Counter();
    APP_DBG("Friends Counter: %d", friendsCounter);

    // Load friends from flash
    Flash_Load_Friends(friends, friendsCounter);
}

void Friends_List()
{
    for (uint16_t i = 0; i < friendsCounter; i++)
    {
        APP_DBG("Friend %d: %02X:%02X:%02X:%02X:%02X:%02X", i,
                friends[i].address[5], friends[i].address[4],
                friends[i].address[3], friends[i].address[2],
                friends[i].address[1], friends[i].address[0]);
    }
}

void Friends_Add(uint8_t nearbyDeviceAddress[B_ADDR_LEN])
{
    if (friendsCounter >= FRIENDS_MAX)
    {
        APP_DBG("Cannot add friend, maximum number of friends reached");
        return;
    }

    // Add friend to array
    // friends[friendsCounter] = friend;
    for (uint8_t i = 0; i < 6; i++)
    {
        friends[friendsCounter].address[i] = nearbyDeviceAddress[i];
    }

    friendsCounter++;
    Flash_Set_Friends_Counter(friendsCounter);

    // Save friends to flash
    Flash_Save_Friends(friends, friendsCounter);
}