/* 
   RU: Библиотека для работы с часами реального времени на микросхеме DS1307
   EN: Library for working with real-time clock on the DS1307 chip
   --------------------------------------------------------------------------------
   (с) 2022 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RE_DS1307_H__
#define __RE_DS1307_H__

#include <stdint.h>
#include <esp_err.h>
#include <sys/time.h> 
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DS1307_ADDR 0x68 //!< I2C address

/**
 * Squarewave frequency
 */
typedef enum {
    DS1307_1HZ = 0, //!< 1 Hz
    DS1307_4096HZ,  //!< 4096 Hz
    DS1307_8192HZ,  //!< 8192 Hz
    DS1307_32768HZ  //!< 32768 Hz
} ds1307_squarewave_freq_t;

/**
 * Real-time clock DS1307
 */
class reDS1307 {
  public:
    reDS1307(i2c_port_t numI2C, uint8_t addrI2C);
    ~reDS1307();

    bool start(bool start = true);
    bool is_running();

    bool get_time(struct tm *time);
    bool set_time(const struct tm *time);

    bool enable_squarewave(bool enable);
    bool is_squarewave_enabled();
    bool set_squarewave_freq(ds1307_squarewave_freq_t freq);
    bool get_squarewave_freq(ds1307_squarewave_freq_t *sqw_freq);
    bool get_output();
    bool set_output(bool value);

    bool read_rom(uint8_t offset, uint8_t *buf, uint8_t len);
    bool write_rom(uint8_t offset, uint8_t *buf, uint8_t len);
  private:
    i2c_port_t _numI2C = I2C_NUM_0; 
    uint8_t _addrI2C = 0;

    bool read_register(uint8_t reg, uint8_t *val, uint8_t size);
    bool write_register(uint8_t reg, uint8_t *val, uint8_t size);
    bool update_register(uint8_t reg, uint8_t mask, uint8_t val);
};

#ifdef __cplusplus
}
#endif

#endif // __RE_DS1307_H__