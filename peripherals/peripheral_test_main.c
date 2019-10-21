#include <stdio.h>
#include "rotary_encoder.h"
#include "i2cLcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

bool button_state_changed()
{
    static bool old_button = false;
    if(BUTTON != old_button)
    {
        BUTTON = old_button;
        return true;
    }
    return false;
}

bool button_select()
{
    static bool second = false;
    if(button_state_changed())
    {
        if(second)
        {
            second = false;
            return false;
        }
        second = true;
        return true;
    }
    return false;
}



void app_main()
{
    esp_task_wdt_add(NULL);
    esp_task_wdt_status(NULL);

    lcd_init(0x27, 22, 21);

    init_rotary_encoder(23, 19, 18);
    printf("Hello world!\n");
    lcd_write_str("Test 2121");
    uint64_t old = ROTARY_POSITION;
    uint64_t selected = 0;
    while(true)
    {
        esp_task_wdt_reset();
        vTaskDelay(10);
        if(ROTARY_POSITION != old)
        {
            lcd_start_printf("%lld ", ROTARY_POSITION);
            old = ROTARY_POSITION;
            if(selected)
            {
                lcd_printf("no. selected: %lld ", selected);
            }
            lcd_printf("\n");
        }
        if(button_select())
        {
            selected = ROTARY_POSITION;
            lcd_start_printf("New Select!\n");
        }
    }
    vTaskDelete(NULL);
}
