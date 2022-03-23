#include "reDS1307.h"
#include "reI2C.h"
#include "rLog.h"
#include "reEvents.h"

static const char* logTAG = "RTC";

#define RAM_SIZE     56
#define I2C_TIMEOUT  1000

#define TIME_REG     0
#define CONTROL_REG  7
#define RAM_REG      8

#define CH_BIT       (1 << 7)
#define HOUR12_BIT   (1 << 6)
#define PM_BIT       (1 << 5)
#define SQWE_BIT     (1 << 4)
#define OUT_BIT      (1 << 7)

#define CH_MASK       0x7f
#define SECONDS_MASK  0x7f
#define HOUR12_MASK   0x1f
#define HOUR24_MASK   0x3f
#define SQWEF_MASK    0xfc
#define SQWE_MASK     0xef
#define OUT_MASK      0x7f

static uint8_t bcd2dec(uint8_t val)
{
  return (val >> 4) * 10 + (val & 0x0f);
}

static uint8_t dec2bcd(uint8_t val)
{
  return ((val / 10) << 4) + (val % 10);
}

reDS1307::reDS1307(i2c_port_t numI2C, uint8_t addrI2C)
{
  _numI2C = numI2C;
  _addrI2C = addrI2C;
}

reDS1307::~reDS1307()
{

}

bool reDS1307::read_register(uint8_t reg, uint8_t *val, uint8_t size)
{
  esp_err_t err = readI2C(_numI2C, _addrI2C, &reg, 1, val, size, 0, I2C_TIMEOUT);
  if (err != ESP_OK) {
    rlog_e(logTAG, "Failed to read DS1307 register #%d: %d (%s)", reg, err, esp_err_to_name(err));
    return false;
  };
  return true;
}

bool reDS1307::write_register(uint8_t reg, uint8_t *val, uint8_t size)
{
  esp_err_t err = writeI2C(_numI2C, _addrI2C, &reg, 1, val, size, I2C_TIMEOUT);
  if (err != ESP_OK) {
    rlog_e(logTAG, "Failed to write DS1307 register #%d: %d (%s)", reg, err, esp_err_to_name(err));
    return false;
  };
  return true;
}

bool reDS1307::update_register(uint8_t reg, uint8_t mask, uint8_t val)
{
  uint8_t old;
  if (read_register(reg, &old, 1)) {
    uint8_t buf = (old & mask) | val;
    if (write_register(reg, &buf, 1)) {
      return true;
    };
  };
  return false;
}

bool reDS1307::start(bool start)
{
  if (update_register(TIME_REG, CH_MASK, start ? 0 : CH_BIT)) {
    if (start) {
      rlog_i(logTAG, "Real time clock DS1307 started");
    } else {
      rlog_i(logTAG, "Real time clock DS1307 stopped");
    };
    return true;
  };
  return false;
}

bool reDS1307::is_running()
{
  uint8_t val;
  if (read_register(TIME_REG, &val, 1)) {
    return val & CH_BIT ? false : true;
  };
  return false;
}

bool reDS1307::get_time(struct tm *time)
{
  uint8_t buf[7];

  if (read_register(TIME_REG, (uint8_t*)&buf, sizeof(buf))) {
    time->tm_sec = bcd2dec(buf[0] & SECONDS_MASK);
    time->tm_min = bcd2dec(buf[1]);
    if (buf[2] & HOUR12_BIT) {
      // RTC in 12-hour mode
      time->tm_hour = bcd2dec(buf[2] & HOUR12_MASK) - 1;
      if (buf[2] & PM_BIT) time->tm_hour += 12;
    } else {
      time->tm_hour = bcd2dec(buf[2] & HOUR24_MASK);
    };
    time->tm_wday = bcd2dec(buf[3]) - 1;
    time->tm_mday = bcd2dec(buf[4]);
    time->tm_mon  = bcd2dec(buf[5]) - 1;
    time->tm_year = bcd2dec(buf[6]) + 100;

    rlog_i(logTAG, "Real time clock DS1307 read: year=%d, month=%d, mday=%d, wday=%d, hour=%d, min=%d, sec=%d",
      time->tm_year - 100, time->tm_mon + 1, time->tm_mday, time->tm_wday + 1, time->tm_hour, time->tm_min, time->tm_sec);
    return true;
  };
  return false;
}

bool reDS1307::set_time(const struct tm *time)
{
  uint8_t buf[7] = {
    dec2bcd(time->tm_sec),
    dec2bcd(time->tm_min),
    dec2bcd(time->tm_hour),
    dec2bcd(time->tm_wday + 1),
    dec2bcd(time->tm_mday),
    dec2bcd(time->tm_mon + 1),
    dec2bcd(time->tm_year - 100)
  };

  if (write_register(TIME_REG, (uint8_t*)&buf, sizeof(buf))) {
    rlog_i(logTAG, "Real time clock DS1307 set: year=%d, month=%d, mday=%d, wday=%d, hour=%d, min=%d, sec=%d",
      time->tm_year - 100, time->tm_mon + 1, time->tm_mday, time->tm_wday + 1, time->tm_hour, time->tm_min, time->tm_sec);
    return true;
  };
  return false;
}

bool reDS1307::enable_squarewave(bool enable)
{
  if (update_register(CONTROL_REG, SQWE_MASK, enable ? SQWE_BIT : 0)) {
    if (enable) {
      rlog_i(logTAG, "DS1307 squarewave enabled");
    } else {
      rlog_i(logTAG, "DS1307 squarewave disabled");
    };
    return true;
  };
  return false;
}

bool reDS1307::is_squarewave_enabled()
{
  uint8_t val;
  if (read_register(CONTROL_REG, &val, 1)) {
    return val & SQWE_BIT;
  };
  return false;
}

bool reDS1307::get_squarewave_freq(ds1307_squarewave_freq_t *sqw_freq)
{
  uint8_t val;
  if (read_register(CONTROL_REG, &val, 1)) {
    *sqw_freq = (ds1307_squarewave_freq_t)(val & ~SQWEF_MASK);
    return true;
  };
  return false;
}

bool reDS1307::set_squarewave_freq(ds1307_squarewave_freq_t freq)
{
  return update_register(CONTROL_REG, SQWEF_MASK, freq);
}

bool reDS1307::get_output()
{
  uint8_t val;
  if (read_register(CONTROL_REG, &val, 1)) {
    return val & OUT_BIT;
  };
  return false;
}

bool reDS1307::set_output(bool value)
{
  return update_register(CONTROL_REG, OUT_MASK, value ? OUT_BIT : 0);  
}

bool reDS1307::read_rom(uint8_t offset, uint8_t *buf, uint8_t len)
{
  if (offset + len > RAM_SIZE)
    return false;
  if (read_register(RAM_REG + offset, buf, len)) {
    return true;
  };
  return false;
}

bool reDS1307::write_rom(uint8_t offset, uint8_t *buf, uint8_t len)
{
  if (offset + len > RAM_SIZE)
    return false;
  if (write_register(RAM_REG + offset, buf, len)) {
    return true;
  };
  return false;
}

