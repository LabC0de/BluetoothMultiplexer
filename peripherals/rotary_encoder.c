#include "rotary_encoder.h"

#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

#define CHECK_ERROR_CODE(returned, expected) ({                        \
            if(returned != expected){                                  \
                printf("TWDT ERROR\n");                                \
                abort();                                               \
            }                                                          \
})

uint64_t ROTARY_POSITION = 0;
bool BUTTON = false;

volatile int64_t cooldown;
volatile int gpio_pins[3];

static xQueueHandle rotary_evt_queue = NULL;

gpio_config_t io_conf;

/**
 * Why interrups for both signal lines?
 * The rotary encoder I was using to bulid this was so noisy that it was impossible to read the base logic of an rotary encoder like intended.
 * but it was generating "characteristic sequences" for either left or right turns that just need to be analyzed and filtered.
 * Why return Characters?
 * They are ideal in encoding small sets of posibilities and are almost as readable as #define's (and they are the smallest unit of data).
 */

static void IRAM_ATTR clk_isr_handler(void* args)
{
    // why set the cooldown here? see switch isr handler
    cooldown = esp_timer_get_time();
    int* pins = (int*)args;
    static char flipflop = 'a';
    char tmp;
    if(gpio_get_level(pins[1]) == 1)
    {
        // tmp == 'A' || 'B'
        tmp = (flipflop - 32);
        xQueueSendFromISR(rotary_evt_queue, &tmp, NULL);
    }
    else
    {
        xQueueSendFromISR(rotary_evt_queue, &flipflop, NULL);
    }
    switch (flipflop)
    {
    case 'a':
        flipflop = 'b';
        break;
    case 'b':
        flipflop = 'a';
        break;
    default:
        ets_printf("\nHOW? clk isr....\n");
        break;
    }
}

static void IRAM_ATTR dt_isr_handler(void* args)
{
    // why set the cooldown here? see switch isr handler
    cooldown = esp_timer_get_time();
    int* pins = (int*)args;
    static char flipflop = 'c';
    char tmp;
    if(gpio_get_level(pins[0]) == 1)
    {
        // tmp == 'C' || 'D'
        tmp = (flipflop - 32);
        xQueueSendFromISR(rotary_evt_queue, &tmp, NULL);
    }
    else
    {
        xQueueSendFromISR(rotary_evt_queue, &flipflop, NULL);
    }
    switch (flipflop)
    {
    case 'c':
        flipflop = 'd';
        break;
    case 'd':
        flipflop = 'c';
        break;
    default:
        ets_printf("\nHOW? dt isr....\n");
        break;
    }
}

/**
 * The switch isr handler definetly needs a cooldown.
 * Sometimes the switch gets triggered just by rotating the encoder.
 * The switch itself is noisy so it tends to get triggered multiple times on release.
 */
static void IRAM_ATTR switch_isr_handler(void* args)
{
    int64_t tmp = esp_timer_get_time();
    if((tmp - cooldown) < 1000)
    {
        return;
    }
    if(BUTTON)
    {
        BUTTON = false;
    }
    else
    {
        BUTTON = true;
    }
    #ifdef DEBUG
        ets_printf("NL\n");
    #endif
    cooldown = tmp;
}

/** 
 * this sequence analyzer could be more strict but its not necessary.
 * It looks for a sequence starter (A/B/C/D) and checks if the right and enough characteristic sequence charcters are existing (a/b/c/d).
 * If so register an increment on the next starter (in that case ender - but they are the same the same).
 */
static char sequence_state_machine(char s)
{
    static char starter = 'f';
    static bool condition1 = false;
    static bool condition2 = false;
    if(condition1 && condition2 && s < 0x61)
    {
        switch(starter)
        {
            case 'A':
            case 'B':
                if(s == 'C' || s == 'D')
                {
                    starter = s;
                    condition1 = false;
                    condition2 = false;
                    ROTARY_POSITION++;
                    return '+';
                }
                break;
            case 'C':
            case 'D':
                if(s == 'A' || s == 'B')
                {
                    starter = s;
                    condition1 = false;
                    condition2 = false;
                    ROTARY_POSITION--;
                    return '-';
                }
                break;
            default:
                break;
        }
    }
    if(s < 0x61)
    {
        starter = s;
        condition1 = false;
        condition2 = false;
    }
    switch (starter)
    {
    case 'A':
        if(s == 'b')
        {
            condition1 = true;
        }
        if(s == 'c' || s == 'd')
        {
            condition2 = true;
        }
        break;
    case 'B':
        if(s == 'a')
        {
            condition1 = true;
        }
        if(s == 'c' || s == 'd')
        {
            condition2 = true;
        }
        break;
    case 'C':
        if(s == 'd')
        {
            condition1 = true;
        }
        if(s == 'a' || s == 'b')
        {
            condition2 = true;
        }
        break;
    case 'D':
        if(s == 'c')
        {
            condition1 = true;
        }
        if(s == 'a' || s == 'b')
        {
            condition2 = true;
        }
        break;
    default:
        break;
    }
    return 0;
} 

static void IRAM_ATTR sequence_analyzer_task(void* arg)
{
    //Subscribe this task to TWDT, then check if it is subscribed
    CHECK_ERROR_CODE(esp_task_wdt_add(NULL), ESP_OK);
    CHECK_ERROR_CODE(esp_task_wdt_status(NULL), ESP_OK);


    char marker;
    for(;;)
    {
        CHECK_ERROR_CODE(esp_task_wdt_reset(), ESP_OK);
        if(xQueueReceive(rotary_evt_queue, &marker, 10))
        {
            marker = sequence_state_machine(marker);
            #ifdef DEBUG
                if(marker)
                {
                    // remember to flush stdout on the esp32 or it will not print
                    printf("%lld ", rotary_position);
                    fflush(stdout);
                }
            #endif
        }
    }
    vTaskDelete(NULL);
}



void init_rotary_encoder(int clk, int dt, int sw)
{
    gpio_pins[0] = clk;
    gpio_pins[1] = dt;
    gpio_pins[2] = sw;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO clk here
    io_conf.pin_bit_mask = (1 << clk) | (1 << dt) | (1 << sw) ;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_set_intr_type(sw, GPIO_INTR_NEGEDGE);

    rotary_evt_queue = xQueueCreate(64, sizeof(char));

    xTaskCreate(sequence_analyzer_task, "rotary_encoder_sequence_analyzer", 2048, NULL, 24, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(clk, clk_isr_handler, (void*)gpio_pins);
    gpio_isr_handler_add(dt, dt_isr_handler, (void*)gpio_pins);
    gpio_isr_handler_add(sw, switch_isr_handler, (void*)gpio_pins);
}

