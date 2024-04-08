
#include "driver/i2c.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include <stdio.h>
#include "ic2Initialize.c"
#include "variables.c"
#include "readAndDisplayAccelValues.c"


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

  while (1)
  {

    int16_t xAccl, yAccl, zAccl;
    readAccel(&xAccl, &yAccl, &zAccl);

    printf("--------------------\n");
    printf("Acceleration in X-Axis : %d\n", xAccl);
    printf("Acceleration in Y-Axis : %d\n", yAccl);
    printf("Acceleration in Z-Axis : %d\n", zAccl);

    vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);
  }
}
