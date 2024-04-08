#include "variables.c"
#include "driver/i2c.h"

void readAccel(int16_t *AccelX, int16_t *AccelY, int16_t *AccelZ)
{
  uint8_t data[7];
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (MMA8452Q_ADDR << 1) | I2C_MASTER_READ, true);
  i2c_master_read(cmd, data, 7, I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, portMAX_DELAY);
  i2c_cmd_link_delete(cmd);

  int16_t x, y, z;
  x = ((data[1] << 8) | data[2]) / 16;
  if (x > 2047)
  {
    x -= 4096;
  }

  y = ((data[3] << 8) | data[4]) / 16;
  if (y > 2047)
  {
    y -= 4096;
  }

  z = ((data[5] << 8) | data[6]) / 16;
  if (z > 2047)
  {
    z -= 4096;
  }

  *AccelX = x;
  *AccelY = y;
  *AccelZ = z;
}


void printValues(int16_t AccelX, int16_t AccelY, int16_t AccelZ){
    printf("--------------------\n");
    printf("Acceleration in X-Axis : %d\n", AccelX);
    printf("Acceleration in Y-Axis : %d\n", AccelY);
    printf("Acceleration in Z-Axis : %d\n", AccelZ);

    vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);
}