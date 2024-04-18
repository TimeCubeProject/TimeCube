#include "driver/i2c.h"

void main_i2c_init(uint8_t MMA8452Q_SDA, uint8_t MMA8452Q_SCL) {
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = MMA8452Q_SDA;
  conf.scl_io_num = MMA8452Q_SCL;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 100000;
  conf.clk_flags = 0;
  i2c_param_config(I2C_NUM_0, &conf);
  i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

void init_Ic2_With_Given_Parameters(uint8_t MMA8452Q_ADDR,
                                    uint8_t register_address,
                                    uint8_t bits_to_set) {

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (MMA8452Q_ADDR << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, register_address, true);
  i2c_master_write_byte(cmd, bits_to_set, true); // StandBy mode
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(I2C_NUM_0, cmd, portMAX_DELAY);
  i2c_cmd_link_delete(cmd);
}