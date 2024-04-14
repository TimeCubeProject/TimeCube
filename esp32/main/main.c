
#include "driver/gpio.h"
#include "driver/i2c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ic2Initialize.c"
#include "readAndDisplayAccelValues.c"
#include "readAndDisplayTimerValue.c"
#include "variables.c"
#include <esp_log.h>
#include <stdio.h>
#include "checkIfCubeChangedPosition.c"
#include "wifi_starter.c"

void MMA8452Q_init()
{
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x2A, 0x00);
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x2A, 0x01);
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x0E, 0x00);
}

void app_main()
{
  main_i2c_init(MMA8452Q_SDA, MMA8452Q_SCL);
  MMA8452Q_init();
  startWifi();

  gptimer_handle_t gptimer = timer_init();

  while (1)
  {
    printf("----------------\nTimer value: %lld\n------------\n",
           getTimerValueMs(gptimer));

    int16_t AccelX, AccelY, AccelZ;
    readAccel(&AccelX, &AccelY, &AccelZ);
    printValues(AccelX, AccelY, AccelZ);
    int wallPosition = checkPosition(AccelX, AccelY, AccelZ);
    if (wallPosition != 0)
    {
      printf("Current wall is %d \n", wallPosition);
    }

    vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);
  }
}
