#include "pins/pins.h"

#ifdef CONFIG_PINS_INTR_NEGEDGE
#define PIN_INTR_TYPE GPIO_INTR_NEGEDGE
#elif defined(CONFIG_PINS_INTR_POSEDGE)
#define PIN_INTR_TYPE GPIO_INTR_POSEDGE
#elif defined(CONFIG_PINS_INTR_ANYEDGE)
#define PIN_INTR_TYPE GPIO_INTR_ANYEDGE
#elif defined(CONFIG_PINS_INTR_LOW_LEVEL)
#define PIN_INTR_TYPE GPIO_INTR_LOW_LEVEL
#elif defined(CONFIG_PINS_INTR_HIGH_LEVEL)
#define PIN_INTR_TYPE GPIO_INTR_HIGH_LEVEL
#else
#define PIN_INTR_TYPE GPIO_INTR_NEGEDGE
#endif

#ifdef CONFIG_PINS_PULLUP_ENABLE
#define PIN_PULLUP GPIO_PULLUP_ENABLE
#else
#define PIN_PULLUP GPIO_PULLUP_DISABLE
#endif

#ifdef CONFIG_PINS_PULLDOWN_ENABLE
#define PIN_PULLDOWN GPIO_PULLDOWN_ENABLE
#else
#define PIN_PULLDOWN GPIO_PULLDOWN_DISABLE
#endif
#define PINS_MAX_INPUT_PINS 10
#include <freertos/FreeRTOS.h>
#include <esp_system.h>
#include <esp_bit_defs.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <driver/rtc_io.h>

typedef struct
{
  gpio_num_t     pin_num;
  pins_handle_t hdl;
} isr_data_t;

struct pins_handle
{
  QueueHandle_t gpio_evt_queue;
  pins_input_pin_t pins[PINS_MAX_INPUT_PINS];
  isr_data_t isr_data[PINS_MAX_INPUT_PINS];
};

static pins_input_pin_t *get_pin(pins_handle_t hdl, gpio_num_t pin_num)
{
  for (int i = 0; i < PINS_MAX_INPUT_PINS; ++i)
  {
    if (hdl->pins[i].gpio_pin_num == pin_num)
    {
      return &hdl->pins[i];
    }
  }
  return NULL;
}

static void handle_button_press(pins_handle_t hdl, gpio_num_t incoming_pin)
{
  pins_input_pin_t *pin = get_pin(hdl, incoming_pin);
  if (unlikely(pin == NULL))
  {
    return;
  }

  pin->interrupt_callback(pin->args);
}

static void pins_task_cb(void* arg)
{
  pins_handle_t hdl = arg;

  while (1)
  {
    gpio_num_t incoming_pin;
    if (xQueueReceive(hdl->gpio_evt_queue, &incoming_pin, portMAX_DELAY))
    {
      handle_button_press(hdl, incoming_pin);
    }
  }
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
  isr_data_t *data = (isr_data_t *)arg;
  gpio_num_t pin_num = data->pin_num;
  pins_handle_t hdl = data->hdl;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xQueueSendFromISR(hdl->gpio_evt_queue, &pin_num, &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR(); 
  }
}

void pins_setup_input_pin(pins_handle_t hdl, pins_input_pin_t *pin)
{

  gpio_config_t button_cfg = {
    .intr_type = PIN_INTR_TYPE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = (1ULL << pin->gpio_pin_num),
    .pull_down_en = PIN_PULLDOWN,
    .pull_up_en = PIN_PULLUP
  };
  gpio_config(&button_cfg);

  int free_idx = -1;
  for (int i = 0; i < PINS_MAX_INPUT_PINS; ++i)
  {
    if (hdl->pins[i].gpio_pin_num == -1)
    {
      free_idx = i;
      break;
    }
  }

  if (free_idx == -1)
  {
    ESP_LOGE("pins", "Couldn't add input pin - limit exceeded");
    return;
  }

  hdl->pins[free_idx] = *pin;
  hdl->isr_data[free_idx].pin_num = pin->gpio_pin_num;
  hdl->isr_data[free_idx].hdl = hdl;

  gpio_isr_handler_add(pin->gpio_pin_num, button_isr_handler, &hdl->isr_data[free_idx]);
}

void pins_setup_input_pins(pins_handle_t hdl, size_t length, pins_input_pin_t *pins)
{
  for (size_t i = 0; i < length; ++i)
  {
    pins_setup_input_pin(hdl, &pins[i]);
  }
}

void pins_remove_input_pin(pins_handle_t hdl, gpio_num_t pin_num)
{
  for (int i = 0; i < PINS_MAX_INPUT_PINS; ++i)
  {
    if (hdl->pins[i].gpio_pin_num == pin_num)
    {
      gpio_isr_handler_remove(pin_num);
      hdl->pins[i].gpio_pin_num = -1;
      return;
    }
  }
}

void pins_remove_input_pins(pins_handle_t hdl, size_t length, gpio_num_t *pin_nums)
{
  for (size_t i = 0; i < length; ++i)
  {
    pins_remove_input_pin(hdl, pin_nums[i]);
  }
}

void pins_remove_all_input_pins(pins_handle_t hdl)
{
  for (int i = 0; i < PINS_MAX_INPUT_PINS; ++i)
  {
    if (hdl->pins[i].gpio_pin_num != -1)
    {
      gpio_isr_handler_remove(hdl->pins[i].gpio_pin_num);
      hdl->pins[i].gpio_pin_num = -1;
    }
  }
}

#define GPIO_EVT_QUEUE_SIZE 10

pins_handle_t pins_init()
{
  pins_handle_t hdl = malloc(sizeof(struct pins_handle));

  esp_err_t err = gpio_install_isr_service(0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE("pins", "couldn't initialise ISR service");
  }

  hdl->gpio_evt_queue = xQueueCreate(GPIO_EVT_QUEUE_SIZE, sizeof(uint32_t));
  configASSERT(hdl->gpio_evt_queue != NULL);

  for (int i = 0; i < PINS_MAX_INPUT_PINS; ++i)
  {
    hdl->pins[i].gpio_pin_num = -1;
  }

  BaseType_t ret = xTaskCreate(pins_task_cb, "pins", 2048, hdl, 1, NULL);
  configASSERT(ret == pdPASS);

  return hdl;
}

void pins_setup_wakeup_pin(pins_input_pin_t *pin)
{
  rtc_gpio_pullup_en(pin->gpio_pin_num);
  rtc_gpio_pulldown_dis(pin->gpio_pin_num);

  esp_sleep_enable_ext0_wakeup(pin->gpio_pin_num, CONFIG_PINS_WAKEUP_LEVEL);
}

const pins_api_t *pins_get_api(void)
{
  static const pins_api_t api = {
    .init             = pins_init,
    .destroy          = pins_destroy,
    .setup_input_pin  = pins_setup_input_pin,
    .setup_input_pins = pins_setup_input_pins,
    .remove_input_pin = pins_remove_input_pin,
    .remove_input_pins = pins_remove_input_pins,
    .remove_all_input_pins = pins_remove_all_input_pins,
    .setup_wakeup_pin = pins_setup_wakeup_pin
  };

  return &api;
}

void pins_destroy(pins_handle_t hdl)
{
  pins_remove_all_input_pins(hdl);
  free(hdl);
}
