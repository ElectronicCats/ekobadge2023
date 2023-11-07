#include "Keyboard.h"

#define WS2812DMA_IMPLEMENTATION
// #define WSRBG //For WS2816C's.
#define WSGRB // For SK6805-EC15
#include "leds.h"

void Keyboard_Print_Button(uint8_t keys)
{
    switch (keys)
    {
    case BUTTON_BACK:
        APP_DBG("Button pressed: BACK");
        break;
    case BUTTON_UP:
        APP_DBG("Button pressed: UP");
        break;
    case BUTTON_DOWN:
        APP_DBG("Button pressed: DOWN");
        break;
    case BUTTON_SELECT:
        APP_DBG("Button pressed: SELECT");
        break;
    default:
        break;
    }
}

void Keyboard_Print_Layer(uint8_t layer)
{
    switch (layer)
    {
    case LAYER_MAIN:
        APP_DBG("Layer main");
        break;
    case LAYER_NEOPIXELS_MENU:
        APP_DBG("Layer neopixels menu");
        break;
    case LAYER_NEOPIXEL_1:
        APP_DBG("Layer neopixel 1");
        break;
    case LAYER_NEOPIXEL_2:
        APP_DBG("Layer neopixel 2");
        break;
    case LAYER_NEOPIXEL_3:
        APP_DBG("Layer neopixel 3");
        break;
    case LAYER_NEOPIXELS_RAINBOW:
        APP_DBG("Layer neopixels raibow");
        break;
    case LAYER_FRIENDS_MENU:
        APP_DBG("Layer friends menu");
        break;
    case LAYER_FRIENDS_SEARCH:
        APP_DBG("Layer friends search");
        break;
    case LAYER_FRIENDS_HELP:
        APP_DBG("Layer friends help");
        break;
    case LAYER_FRIENDS_MENU_UNLOCKED:
        APP_DBG("Layer menu unlocked");
        break;
    case LAYER_PROPERTIES:
        APP_DBG("Layer properties");
        break;
    case LAYER_CREDITS:
        APP_DBG("Layer credits");
        break;
    case LAYER_SENSOR_MENU:
        APP_DBG("Layer sensor menu");
        break;
    case LAYER_SENSOR_QUESTION:
        APP_DBG("Layer sensor question");
        break;
    case LAYER_SENSOR_HELP:
        APP_DBG("Layer sensor help");
        break;
    case LAYER_WRONG_YEAR:
        APP_DBG("Layer wrong year");
        break;
    case LAYER_CORRECT_YEAR:
        APP_DBG("Layer correct year");
        break;
    case LAYER_SENSOR_MENU_UNLOCKED:
        APP_DBG("Layer sensor menu unlocked");
        break;
    case LAYER_SECRET_BANNER:
        APP_DBG("Layer secret banner");
        break;
    default:
        APP_DBG("Missing layer");
        break;
    }
}

void Keyboard_Scan_Callback(uint8_t keys)
{
    // Keyboard_Print_Button(keys);

    switch (keys)
    {
    case BUTTON_BACK:
        Keyboard_Handle_Back_Button();
        break;
    case BUTTON_UP:
        selectedOption--;

        if (selectedOption == 255) // Underflow
            selectedOption = 0;

        Display_Update_Menu();
        break;
    case BUTTON_DOWN:
        selectedOption++;

        if (selectedOption > optionsSize - 1)
            selectedOption = optionsSize - 1;

        Display_Update_Menu();
        break;
    case BUTTON_SELECT:
        switch (currentLayer)
        {
        case LAYER_MAIN:
            Main_Menu();
            break;
        case LAYER_NEOPIXELS_MENU:
            Neopixels_Menu();
            break;
        case LAYER_NEOPIXEL_1:
        case LAYER_NEOPIXEL_2:
        case LAYER_NEOPIXEL_3:
            Neopixel_Options();
            break;
        case LAYER_FRIENDS_MENU:
            Friends_Menu();
            break;
        case LAYER_FRIENDS_SEARCH:
            break;
        case LAYER_FRIENDS_HELP:
        case LAYER_FRIEND_FOUND:
        case LAYER_GET_50_FRIENDS:
            Friend_Option_Ok();
            break;
        case LAYER_FRIENDS_MENU_UNLOCKED:
            Friend_Menu_Unlocked();
            break;
        case LAYER_PROPERTIES:
            Properties_Menu();
            break;
        case LAYER_SENSOR_MENU:
            Sensor_Menu();
            break;
        case LAYER_SENSOR_QUESTION:
            Sensor_Question_Menu();
            break;
        case LAYER_WRONG_YEAR:
            Sensor_Wrong_Year();
            break;
        case LAYER_CORRECT_YEAR:
            Sensor_Correct_Year();
            break;
        case LAYER_SENSOR_MENU_UNLOCKED:
            Sensor_Menu_Unlocked();
            break;
        case LAYER_FINISH_LEVEL_1:
            Finish_Level_1();
            break;
        case LAYER_BANNER_LEVEL_2:
            Banner_Level_2();
            break;
        case LAYER_BANNER_LEVEL_3:
            Banner_Level_3();
            break;
        case LAYER_WELCOME_LEVEL_2:
            Welcome_Level_2();
            break;
        case LAYER_WELCOME_LEVEL_3:
            Welcome_Level_3();
            break;
        case LAYER_FINISH_LEVEL_2:
            Finish_Level_2();
            break;
        case LAYER_LEVELS_MENU:
            Levels_Menu();
            break;
        case LAYER_LEVEL_1_OPTONS:
        case LAYER_LEVEL_2_OPTONS:
        case LAYER_LEVEL_3_OPTONS:
            Levels_Options();
        }
        break;
    default:
        break;
    }

    Update_Previous_Layer();
    // Keyboard_Print_Layer(previousLayer);
    // Keyboard_Print_Layer(currentLayer);
    // APP_DBG("selectedOption %d", selectedOption);
}

void Keyboard_Handle_Back_Button()
{
    switch (currentLayer)
    {
    case LAYER_FRIENDS_SEARCH:
        enableFriendSearch = FALSE;
        break;
    case LAYER_SENSOR_QUESTION:
        Disable_Receive_Data();
        break;
    case LAYER_SECRET_BANNER:
        tmos_stop_task(displayTaskID, DISPLAY_SEND_SECRET_EVENT);
        tmos_stop_task(displayTaskID, DISPLAY_RECEIVE_SECRET_EVENT);
        break;
    default:
        break;
    }

    currentLayer = previousLayer;
    selectedOption = 0;
    Keyboard_Update_Orientation();
    Display_Update_Menu();
}

void Keyboard_Update_Orientation()
{
    switch (currentLayer)
    {
    case LAYER_FRIENDS_SEARCH:
    case LAYER_FRIENDS_HELP:
    case LAYER_FRIENDS_MENU_UNLOCKED:
    case LAYER_SENSOR_MENU_UNLOCKED:
    case LAYER_CORRECT_YEAR:
    case LAYER_BANNER_LEVEL_2:
    case LAYER_BANNER_LEVEL_3:
        menuOrientation = HORIZONTAL_MENU;
        break;
    default: // Most of layers are vertical menus
        menuOrientation = VERTICAL_MENU;
        break;
    }
}

void Update_Previous_Layer()
{
    switch (currentLayer)
    {
    case LAYER_MAIN:
    case LAYER_NEOPIXELS_MENU:
    case LAYER_FRIENDS_MENU:
    case LAYER_PROPERTIES:
    case LAYER_SENSOR_MENU:
    case LAYER_SECRET_BANNER:
    case LAYER_LEVELS_MENU:
        previousLayer = LAYER_MAIN;
        break;
    case LAYER_NEOPIXEL_1:
    case LAYER_NEOPIXEL_2:
    case LAYER_NEOPIXEL_3:
    case LAYER_NEOPIXELS_RAINBOW:
        previousLayer = LAYER_NEOPIXELS_MENU;
        break;
    case LAYER_FRIENDS_SEARCH:
    case LAYER_FRIENDS_HELP:
    case LAYER_FRIEND_FOUND:
        previousLayer = LAYER_FRIENDS_MENU;
        break;
    case LAYER_FRIENDS_MENU_UNLOCKED:
        previousLayer = LAYER_FRIENDS_MENU_UNLOCKED;
        break;
    case LAYER_CREDITS:
        previousLayer = LAYER_PROPERTIES;
        break;
    case LAYER_SENSOR_QUESTION:
    case LAYER_SENSOR_HELP:
        previousLayer = LAYER_SENSOR_MENU;
        break;
    case LAYER_WRONG_YEAR:
        previousLayer = LAYER_SENSOR_QUESTION;
        break;
    case LAYER_CORRECT_YEAR:
        previousLayer = LAYER_CORRECT_YEAR;
        break;
    case LAYER_SENSOR_MENU_UNLOCKED:
        previousLayer = LAYER_SENSOR_MENU_UNLOCKED;
        break;
    case LAYER_FINISH_LEVEL_1:
        previousLayer = LAYER_FINISH_LEVEL_1;
        break;
    case LAYER_BANNER_LEVEL_2:
        previousLayer = LAYER_BANNER_LEVEL_2;
        break;
    case LAYER_BANNER_LEVEL_3:
        previousLayer = LAYER_BANNER_LEVEL_3;
        break;
    case LAYER_WELCOME_LEVEL_2:
        previousLayer = LAYER_WELCOME_LEVEL_2;
        break;
    case LAYER_FINISH_LEVEL_2:
        previousLayer = LAYER_FINISH_LEVEL_2;
        break;
    case LAYER_WELCOME_LEVEL_3:
        previousLayer = LAYER_WELCOME_LEVEL_3;
        break;
    case LAYER_LEVEL_1_OPTONS:
    case LAYER_LEVEL_2_OPTONS:
    case LAYER_LEVEL_3_OPTONS:
        previousLayer = LAYER_LEVELS_MENU;
        break;
    case LAYER_LEVEL_1_HELP:
        previousLayer = LAYER_LEVEL_1_OPTONS;
        break;
    case LAYER_LEVEL_2_HELP:
        previousLayer = LAYER_LEVEL_2_OPTONS;
        break;
    case LAYER_LEVEL_3_HELP:
        previousLayer = LAYER_LEVEL_3_OPTONS;
        break;
    default:
        previousLayer = LAYER_MAIN;
        APP_DBG("Unknown layer:");
        Keyboard_Print_Layer(currentLayer);
        break;
    }
}

void Main_Menu()
{
    BOOL vertical = TRUE;

    switch (selectedOption)
    {
    case MAIN_NEOPIXELS_MENU:
        currentLayer = LAYER_NEOPIXELS_MENU;
        break;
    case MAIN_LEVELS_MENU:
        currentLayer = LAYER_LEVELS_MENU;
        break;
    case MAIN_FRIENDS_MENU:
        currentLayer = LAYER_FRIENDS_MENU;
        break;
    case MAIN_PROPERTIES_MENU:
        currentLayer = LAYER_PROPERTIES;
        break;
    case MAIN_SENSOR_MENU:
        currentLayer = LAYER_SENSOR_MENU;
        break;
    case MAIN_SECRET_BANNER:
        currentLayer = LAYER_SECRET_BANNER;
        tmos_start_task(displayTaskID, DISPLAY_RECEIVE_SECRET_EVENT, MS1_TO_SYSTEM_TIME(NO_DELAY));
        tmos_start_task(displayTaskID, DISPLAY_SEND_SECRET_EVENT, MS1_TO_SYSTEM_TIME(SEND_SECRET_DELAY));
        vertical = FALSE;
        break;
    default:
        APP_DBG("Missing option");
        break;
    }

    selectedOption = 0;

    if (vertical)
        Display_Update_VMenu();
    else
        Display_Update_HMenu();
}

void Levels_Menu()
{
    switch (selectedOption)
    {
    case LEVEL_1:
        currentLayer = LAYER_LEVEL_1_OPTONS;
        break;
    case LEVEL_2:
        currentLayer = LAYER_LEVEL_2_OPTONS;
        break;
    case LEVEL_3:
        currentLayer = LAYER_LEVEL_3_OPTONS;
        break;
    default:
        APP_DBG("Missing option");
        break;
    }

    selectedOption = 0;
    Display_Update_VMenu();
}

Levels_Options()
{
    BOOL vertical = TRUE; // for level 1

    switch (currentLayer)
    {
    case LAYER_LEVEL_1_OPTONS:
        switch (selectedOption)
        {
        case LEVELS_HELP:
            currentLayer = LAYER_LEVEL_1_HELP;
            vertical = TRUE;
            break;
        default:
            APP_DBG("Missing option");
            break;
        }
        break;
    case LAYER_LEVEL_2_OPTONS:
        switch (selectedOption)
        {
        case LEVELS_HELP:
            currentLayer = LAYER_LEVEL_2_HELP;
            vertical = TRUE;
            break;
        default:
            APP_DBG("Missing option");
            break;
        }
        break;
    case LAYER_LEVEL_3_OPTONS:
        switch (selectedOption)
        {
        case LEVELS_HELP:
            currentLayer = LAYER_LEVEL_3_HELP;
            vertical = TRUE;
            break;
        default:
            APP_DBG("Missing option");
            break;
        }
        break;
    }

    selectedOption = 0;

    if (vertical)
        Display_Update_VMenu();
    else
        Display_Update_HMenu();
}

void Neopixels_Menu()
{
    switch (selectedOption)
    {
    case NEOPIXEL_1:
        currentLayer = LAYER_NEOPIXEL_1;
        break;
    case NEOPIXEL_2:
        currentLayer = LAYER_NEOPIXEL_2;
        break;
    case NEOPIXEL_3:
        currentLayer = LAYER_NEOPIXEL_3;
        break;
    case NEOPIXELS_RAINBOW:
        // currentLayer = LAYER_NEOPIXELS_RAINBOW;
        tmos_start_task(ledsTaskID, LEDS_RAINBOW_EVENT, MS1_TO_SYSTEM_TIME(RAINBOW_DELAY_MS));
        break;
    default:
        APP_DBG("Missing option: %d", selectedOption);
        break;
    }

    selectedOption = 0;
    Display_Update_VMenu();
}

void Neopixel_Options()
{
    switch (selectedOption)
    {
    case NEOPIXEL_SET_RED:
        if (currentLayer == LAYER_NEOPIXEL_1)
        {
            Led1_Set_Red();
        }
        else if (currentLayer == LAYER_NEOPIXEL_2)
        {
            Led2_Set_Red();
        }
        else if (currentLayer == LAYER_NEOPIXEL_3)
        {
            Led3_Set_Red();
        }
        break;
    case NEOPIXEL_SET_GREEN:
        if (currentLayer == LAYER_NEOPIXEL_1)
        {
            Led1_Set_Green();
        }
        else if (currentLayer == LAYER_NEOPIXEL_2)
        {
            Led2_Set_Green();
        }
        else if (currentLayer == LAYER_NEOPIXEL_3)
        {
            Led3_Set_Green();
        }
        break;
    case NEOPIXEL_SET_BLUE:
        if (currentLayer == LAYER_NEOPIXEL_1)
        {
            Led1_Set_Blue();
        }
        else if (currentLayer == LAYER_NEOPIXEL_2)
        {
            Led2_Set_Blue();
        }
        else if (currentLayer == LAYER_NEOPIXEL_3)
        {
            Led3_Set_Blue();
        }
        break;
    case NEOPIXEL_ON:
        if (currentLayer == LAYER_NEOPIXEL_1)
        {
            Led1_On();
        }
        else if (currentLayer == LAYER_NEOPIXEL_2)
        {
            Led2_On();
        }
        else if (currentLayer == LAYER_NEOPIXEL_3)
        {
            Led3_On();
        }
        break;
    case NEOPIXEL_OFF:
        if (currentLayer == LAYER_NEOPIXEL_1)
        {
            Led1_Off();
        }
        else if (currentLayer == LAYER_NEOPIXEL_2)
        {
            Led2_Off();
        }
        else if (currentLayer == LAYER_NEOPIXEL_3)
        {
            Led3_Off();
        }
        break;
    default:
        APP_DBG("Missing option");
        break;
    }
}

void Friends_Menu()
{
    BOOL update = TRUE;

    switch (selectedOption)
    {
    case FRIENDS_COUNTER:
        update = FALSE;
        break;
    case FRIENDS_SEARCH:
        menuOrientation = HORIZONTAL_MENU;
        currentLayer = LAYER_FRIENDS_SEARCH;
        enableFriendSearch = TRUE;
        break;
    case FRIENDS_HELP:
        menuOrientation = HORIZONTAL_MENU;
        currentLayer = LAYER_FRIENDS_HELP;
        break;
    default:
        APP_DBG("Missing option: %d", selectedOption);
        break;
    }

    if (!update)
        return;

    selectedOption = 0;
    Display_Update_HMenu();
}

void Friend_Option_Ok()
{
    switch (selectedOption)
    {
    case OK:
        currentLayer = LAYER_FRIENDS_MENU;
        break;
    }

    selectedOption = 0;
    Display_Update_VMenu();
}

void Friend_Menu_Unlocked()
{
    switch (selectedOption)
    {
    case OK:
        currentLayer = LAYER_MAIN;
        break;
    }

    selectedOption = MAIN_SENSOR_MENU; // Select sensor option
    Display_Update_VMenu();
}

void Properties_Menu()
{
    switch (selectedOption)
    {
    case PROPERTIES_CREDITS:
        currentLayer = LAYER_CREDITS;
        break;
    }

    selectedOption = 0;
    Display_Update_VMenu();
}

void Sensor_Menu()
{
    switch (selectedOption)
    {
    case SENSOR_QUESTION:
        currentLayer = LAYER_SENSOR_QUESTION;
        selectedOption = 0;
        Display_Update_HMenu();
        Enable_Receive_Data();
        break;
    case SENSOR_HELP:
        currentLayer = LAYER_SENSOR_HELP;
        selectedOption = 0;
        Display_Update_VMenu();
        break;
    }
}

void Sensor_Question_Menu()
{
    switch (selectedOption)
    {
    case OK:
        Sensor_Answer();
        break;
    case CANCEL:
        currentLayer = LAYER_SENSOR_MENU;
        selectedOption = 0;
        Display_Update_VMenu();
        break;
    }

    Disable_Receive_Data();
}

void Sensor_Answer()
{
    if (Get_Foundation_Year() == FOUNDATION_YEAR)
    {
        currentLayer = LAYER_CORRECT_YEAR;
        Set_Level(3);
    }
    else
    {
        currentLayer = LAYER_WRONG_YEAR;
    }

    selectedOption = 0;
    Display_Update_HMenu();
}

void Sensor_Wrong_Year()
{
    switch (selectedOption)
    {
    case OK:
        currentLayer = LAYER_SENSOR_QUESTION;
        Enable_Receive_Data();
        break;
    }

    selectedOption = 0;
    Display_Update_HMenu();
}

void Sensor_Correct_Year()
{
    switch (selectedOption)
    {
    case OK:
        currentLayer = LAYER_FINISH_LEVEL_2;
        break;
    }

    selectedOption = 0;
    Display_Update_VMenu();
}

void Sensor_Menu_Unlocked()
{
    switch (selectedOption)
    {
    case OK:
        currentLayer = LAYER_MAIN;
        break;
    }

    selectedOption = MAIN_SECRET_BANNER; // Select secret option
    Display_Update_VMenu();
}

// Finishing level 1 shows the level 2 banner
void Finish_Level_1()
{
    switch (selectedOption)
    {
    case OK:
    default:
        currentLayer = LAYER_BANNER_LEVEL_2;
        break;
    }

    selectedOption = 0;
    Display_Update_HMenu();
}

// Press OK show the level 2 welcome
void Banner_Level_2()
{
    switch (selectedOption)
    {
    case OK:
    default:
        currentLayer = LAYER_WELCOME_LEVEL_2;
        break;
    }

    selectedOption = 0;
    Display_Update_VMenu();
}

Welcome_Level_2()
{
    currentLayer = LAYER_FRIENDS_MENU_UNLOCKED;
    selectedOption = 0;
    Display_Update_HMenu();
}

// Finishing level 2 shows the level 3 banner
void Finish_Level_2()
{
    switch (selectedOption)
    {
    case OK:
    default:
        currentLayer = LAYER_BANNER_LEVEL_3;
        break;
    }

    selectedOption = 0;
    Display_Update_HMenu();
}

// Press OK show the level 3 welcome
void Banner_Level_3()
{
    switch (selectedOption)
    {
    case OK:
    default:
        currentLayer = LAYER_WELCOME_LEVEL_3;
        break;
    }

    selectedOption = 0;
    Display_Update_VMenu();
}

Welcome_Level_3()
{
    currentLayer = LAYER_SENSOR_MENU_UNLOCKED;
    selectedOption = 0;
    Display_Update_HMenu();
}
