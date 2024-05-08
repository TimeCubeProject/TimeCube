#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ic2Initialize.c"
#include "readAndDisplayTimerValue.c"
#include "wifi_starter.c"
#include "accelReadAndCheckPosition.c"
#include <stdio.h>
#include "sender.c"

static const char *TAGm = "Accel";

gptimer_handle_t gptimer;
gptimer_handle_t gptimerGlobal;
u_int8_t wallPositionTab[5] = {0, 0, 0, 0, 0};
u_int8_t wallTimeTab[6] = {0, 0, 0, 0, 0, 0};
int16_t AccelX, AccelY, AccelZ;
bool isWallPositionUnchanged;
bool wallSend;

void MMA8452Q_init()
{
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x2A, 0x00);
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x2A, 0x01);
  init_Ic2_With_Given_Parameters(MMA8452Q_ADDR, 0x0E, 0x00);
}

void checkIfPositionWallHasChanged(u_int16_t timerValue)
{
  if (timerValue > 5000)
  {
    wallPositionTab[4] = checkPosition(AccelX, AccelY, AccelZ);
    printValues(AccelX, AccelY, AccelZ);
    resetTimer(gptimer);
    for (size_t i = 0; i < 5; i++)
    {
      printf("Wall postion %d\n", wallPositionTab[i]);
      if (wallPositionTab[i] != wallPositionTab[0])
      {
        printf("Wall position changed\n");
        isWallPositionUnchanged = false;
      }
    }
  }
  else if (timerValue > 4000)
  {
    wallPositionTab[3] = checkPosition(AccelX, AccelY, AccelZ);
    isWallPositionUnchanged = false;
  }
  else if (timerValue > 3000)
  {
    wallPositionTab[2] = checkPosition(AccelX, AccelY, AccelZ);
    isWallPositionUnchanged = false;
  }
  else if (timerValue > 2000)
  {
    wallPositionTab[1] = checkPosition(AccelX, AccelY, AccelZ);
    isWallPositionUnchanged = false;
  }
  else if (timerValue > 1000)
  {
    wallPositionTab[0] = checkPosition(AccelX, AccelY, AccelZ);
    isWallPositionUnchanged = false;
  }
}

void countTimeOnCurrentWall()
{
  if (isWallPositionUnchanged == true && wallPositionTab[0] != 0)
  {

    wallSend = true;
    resetTimer(gptimerGlobal);
    wallTimeTab[wallPositionTab[0] - 1] += 5;
    printf("\n--------------------\n");
    for (size_t i = 0; i <= 5; i++)
    {
      printf("Wall %d time %d\n", i + 1, wallTimeTab[i]);
    }
  }
}

void resetTheWallSendAndPositionTab()
{
  if (wallSend == true)
  {
    for (size_t i = 0; i < 5; i++)
    {
      wallPositionTab[i] = 0;
    }
    wallSend = false;
  }
}

void app_main()
{
  main_i2c_init(MMA8452Q_SDA, MMA8452Q_SCL);
  MMA8452Q_init();
  startWifi();
  rest_get();

  gptimer = timer_init();
  gptimerGlobal = timer_init();

  while (1)
  {
    readAccel(&AccelX, &AccelY, &AccelZ);
    u_int16_t timerValue = getTimerValueMs(gptimer);
    isWallPositionUnchanged = true;

    checkIfPositionWallHasChanged(timerValue);
    wallSend = false;
    countTimeOnCurrentWall();
    resetTheWallSendAndPositionTab();
  }
}
