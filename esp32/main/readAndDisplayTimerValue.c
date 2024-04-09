#include "driver/gptimer.h"

gptimer_handle_t timer_init() {
  printf("Create timer handle");
  gptimer_handle_t gptimer = NULL;
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = 1000000, // 1MHz, 1 tick=1us
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
  ESP_ERROR_CHECK(gptimer_enable(gptimer));
  ESP_ERROR_CHECK(gptimer_start(gptimer));
  ESP_ERROR_CHECK(gptimer_set_raw_count(gptimer, 0));
  return gptimer;
}

uint64_t getTimerValueMs(gptimer_handle_t gptimer) {
  uint64_t count;
  gptimer_get_raw_count(gptimer, &count);
  count = count / 1000; // convert to ms
  return count;
}