/* 
 * LiFePO4wered/Pi access module
 * Copyright (C) 2015-2017 Patrick Van Oosterwijck
 * Released under the GPL v2
 */

#define _DEFAULT_SOURCE
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/file.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "lifepo4wered-access.h"


/* LiFePO4wered/Pi access constants */

#define I2C_BUS             1
#define I2C_ADDRESS         0x43


/* Open access to the specified I2C bus */

static bool open_i2c_bus(int bus, int *file) {
  /* Create the name of the device file */
  char filename[20];
  snprintf(filename, 19, "/dev/i2c-%d", bus);
  /* Open the device file */
  *file = open(filename, O_RDWR);
  if (*file < 0) return false;
  /* Lock access */
  if (flock(*file, LOCK_EX|LOCK_NB) != 0) {
    close (*file);
    return false;
  }
  /* Success */
  return true;
}

/* Close access to the specified I2C bus */

static bool close_i2c_bus(int file) {
  flock(file, LOCK_UN);
  close(file);
  return file >= 0;
}

/* Read LiFePO4wered/Pi data */

bool read_lifepo4wered_data(uint8_t reg, uint8_t count, uint8_t *data) {
  /* Open the I2C bus */
  int file;
  if (!open_i2c_bus(I2C_BUS, &file))
    return false;

  /* Declare I2C message structures */
  struct i2c_msg dwrite, dread;
  struct i2c_rdwr_ioctl_data msgreg = {
    &dwrite,
    1
  };
  struct i2c_rdwr_ioctl_data msgread = {
    &dread,
    1
  };
  /* Write register message */
  dwrite.addr = I2C_ADDRESS;
  dwrite.flags = 0;
  dwrite.len = 1;
  dwrite.buf = &reg;
  /* Read data message */
  dread.addr = I2C_ADDRESS;
  dread.flags = I2C_M_RD;
  dread.len = count;
  dread.buf = data;

  /* Execute the command to send the register */
  bool result = ioctl(file, I2C_RDWR, &msgreg) >= 0;

  /* Delay to ensure the micro detected STOP condition */
  usleep(20);

  /* Execute the command to read data */
  result &= ioctl(file, I2C_RDWR, &msgread) >= 0;

  /* Close the I2C bus */
  close_i2c_bus(file);

  /* Return the result */
  return result;
}

/* Write LiFePO4wered/Pi chip data */

bool write_lifepo4wered_data(uint8_t reg, uint8_t count, uint8_t *data) {
  /* Open the I2C bus */
  int file;
  if (!open_i2c_bus(I2C_BUS, &file))
    return false;

  /* Declare I2C message structures */
  struct i2c_msg dwrite;
  struct i2c_rdwr_ioctl_data msgwrite = {
    &dwrite,
    1
  };
  /* Message payload */
  uint8_t payload[256];
  payload[0] = reg;
  memcpy(&payload[1], data, count);
  /* Write data message */
  dwrite.addr = I2C_ADDRESS;
  dwrite.flags = 0;
  dwrite.len = 1 + count;
  dwrite.buf = payload;

  /* Execute the command */
  bool result = ioctl(file, I2C_RDWR, &msgwrite) >= 0;

  /* Close the I2C bus */
  close_i2c_bus(file);

  /* Return the result */
  return result;
}
