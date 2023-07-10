#define GPIO_INPUT_IO_0 4
#define GPIO_INPUT_PIN_SEL (1ULL << GPIO_INPUT_IO_0)


void IRAM_ATTR wifi_reset_interrupt_handler(void *args);
void LED_Control_Task(void *params);

extern xQueueHandle interputQueue;