/*
 * ADS111x Library for ESP-IDF
 * Author: Shalihuddin Al Fatah
 */

#ifndef ADS111X_H
#define ADS111X_H

#include <stdint.h>
#include <esp_err.h>

#include <driver/i2c_master.h>

#define DEBUG

/*
 * Pointer register address
 * Low threshold and high threshold not available in ADS1113
 */
#define ADS111x_CONV_REG      0x00 // Conversion result
#define ADS111x_CFG_REG       0x01 // Device configuration
#define ADS111x_LO_THRESH_REG 0x02 // Low threshold value
#define ADS111x_HI_THRESH_REG 0x03 // High threshold value

/*
 * Multiplexer configuration
 * @note This item is not configurable in ADS1113 and ADS1114. They always use default value.
 */
#define ADS111x_MUX_DIFF_AIN0_AIN1 0x00 // Default
#define ADS111x_MUX_DIFF_AIN0_AIN3 0x01
#define ADS111x_MUX_DIFF_AIN1_AIN3 0x02
#define ADS111x_MUX_DIFF_AIN2_AIN3 0x03
#define ADS111x_MUX_SNGL_AIN0_GND  0x04
#define ADS111x_MUX_SNGL_AIN1_GND  0x05
#define ADS111x_MUX_SNGL_AIN2_GND  0x06
#define ADS111x_MUX_SNGL_AIN3_GND  0x07

/*
 * Programmable gain amplifier configuration
 * @note This item is not configurable in ADS1113. It always use default value.
 */
#define ADS111x_FSR_6V144 0x00 
#define ADS111x_FSR_4V096 0x01
#define ADS111x_FSR_2V048 0x02 // Default
#define ADS111x_FSR_1V024 0x03
#define ADS111x_FSR_0V512 0x04
#define ADS111x_FSR_0V256 0x05

/*
 * Device operating mode
 */
#define ADS111x_CONT_MEASURE false
#define ADS111x_SINGLE_SHOT  true  // Default

/*
 * Data rate configuration
 * More samples = faster, less accurate. Less samples = slower, more accurate.
 */
#define ADS111x_DR_8SPS   0x00 
#define ADS111x_DR_16SPS  0x01
#define ADS111x_DR_32SPS  0x02
#define ADS111x_DR_64SPS  0x03
#define ADS111x_DR_128SPS 0x04 // Default
#define ADS111x_DR_250SPS 0x05
#define ADS111x_DR_475SPS 0x06
#define ADS111x_DR_860SPS 0x07

/*
 * Comparator mode configuration
 * @note This item is not available for ADS1113
 */
#define ADS111x_TRADITIONAL_COMP false // Default
#define ADS111x_WINDOW_COMP      true

/*
 * Comparator polarity configuration
 * @note This item is not available for ADS1113
 */
#define ADS111x_COMP_ACTIVE_LOW  false // Default
#define ADS111x_COMP_ACTIVE_HIGH true

/*
 * Comparator latching configuration
 * @note This item is not available for ADS1113
 */
#define ADS111x_COMP_NON_LATCHING false // Default
#define ADS111x_COMP_LATCHING     true

/*
 * Comparator queue configuration
 * @note This item is not available for ADS1113
 */
#define ADS111x_COMP_QUEUE_ONE     0x00 
#define ADS111x_COMP_QUEUE_TWO     0x01
#define ADS111x_COMP_QUEUE_FOUR    0x02
#define ADS111x_COMP_QUEUE_DISABLE 0x03 // Default

/*
 * Alert/ready pin configuration  
 */
#define ADS111x_PIN_ALERT      false // Default
#define ADS111x_PIN_CONV_READY true

/*
 * Device configuration structure
 */ 
typedef struct {
   /* ---- Managed by the library ---- */
   uint8_t device_addr;    
   uint8_t buffer[3];        // I2C communication buffer
   float PGA_float;          // Gain amplifier value in float
   uint8_t MSB_config_data; 
   uint8_t LSB_config_data;
   esp_err_t ADS111x_err;    // Error code

   /* ---- Managed by user ---- */
   uint8_t mux_config;
   uint8_t gain_amp;         // Gain amplifier register config
   bool operating_mode;
   uint8_t data_rate;
   bool comp_mode;           // Comparator mode
   bool comp_pol;            // Comparator polarity
   bool comp_latch;          // Comparator latching
   uint8_t comp_queue;       // Comparator queue
   uint16_t low_threshold;   // Low threshold register 
   uint16_t high_threshold;  // High threshold register
   i2c_master_dev_handle_t ads111x_i2c_dev_handle; 
} ads111x_cfg_t;

/*
 * Reset the device configuration structure and put default device config register value from datasheet
 * @param Address of device configuration structure
 * @note Ideally this should just run once
 */
void ads111x_reset_config_reg(ads111x_cfg_t* device_cfg);

/*
 * Helper function to read from specific register 
 * Register value will be placed inside device_cfg->buffer[0] and device_cfg->buffer[1]
 * @param Address of device configuration structure
 * @param Pointer register address macro
 */
void ads111x_read_from_reg(ads111x_cfg_t* device_cfg, uint8_t device_reg);

/*
 * Device address
 * Device address can be configured based on where the ADDR pin is connected to
 */
typedef enum {
    ADDRPIN_TO_GND, // 0x48 1001000b
    ADDRPIN_TO_VCC, // 0x49 1001001b
    ADDRPIN_TO_SDA, // 0x4A 1001010b
    ADDRPIN_TO_SCL  // 0x4B 1001011b
} ads111x_address_e;

/*
 * Configure device address
 * @param Device address enum
 * @param Address of device configuration structure
 * @return ESP_OK success, ESP_ERR_INVALID_ARG invalid device address enum
 */
esp_err_t ads111x_configure_address(ads111x_address_e addr, ads111x_cfg_t* device_cfg);

/* 
 * Function: 
 * 1. ads111x_mux_config
 * 2. ads111x_gain_amp
 * 3. ads111x_operating_mode
 * 4. ads111x_data_rate
 * 5. ads111x_comp_mode
 * 6. ads111x_comp_polarity
 * 7. ads111x_comp_latch
 * 8. ads111x_comp_queue
 * All functions above can be used to change the device setting on the fly. You just need to "set" the otf flag        
 */

/*
 * Configure multiplexer
 * @param Multiplexer configuration macro
 * @param Address of device configuration structure
 * @param Change multiplexer setting on the fly flag
 * @note This function is not available in ADS1113 and ADS1114. They always use default value.
 */
void ads111x_mux_config(uint8_t my_arg, ads111x_cfg_t* device_cfg, bool otf);

/*
 * Configure gain amplifier
 * @param Programmable gain amplifier macro
 * @param Address of device configuration structure
 * @param Change gain amplifier on the fly flag
 * @note This function is not available in ADS1113. It always use default value.
 */
void ads111x_gain_amp(uint8_t my_arg, ads111x_cfg_t* device_cfg, bool otf);

/*
 * Configure device operating mode
 * @param Device operating mode macro
 * @param Address of device configuration structure
 * @param Change operating mode on the fly flag
 */
void ads111x_operating_mode(bool my_arg, ads111x_cfg_t* device_cfg, bool otf);

/*
 * Configure data rate
 * @param Data rate macro
 * @param Address of device configuration structure
 * @param Change data rate setting on the fly flag
 */
void ads111x_data_rate(uint8_t my_arg, ads111x_cfg_t* device_cfg, bool otf);

/*
 * Configure comparator mode
 * @param Comparator mode macro
 * @param Address of device configuration structure
 * @param Change comparator mode on the fly flag
 * @note This function is not available for ADS1113
 */
void ads111x_comp_mode(bool my_arg, ads111x_cfg_t* device_cfg, bool otf);

/*
 * Configure comparator polarity
 * @param Comparator polarity macro
 * @param Address of device configuration structure
 * @param Change comparator polarity on the fly flag
 * @note This function is not available for ADS1113
 */
void ads111x_comp_polarity(bool my_arg, ads111x_cfg_t* device_cfg, bool otf);

/*
 * Configure comparator latching
 * @param Comparator latching macro
 * @param Address of device configuration structure
 * @param Change comparator latching on the fly flag
 * @note This function is not available for ADS1113 
 */
void ads111x_comp_latch(bool my_arg, ads111x_cfg_t* device_cfg, bool otf);

/*
 * Configure comparator queue
 * @param Comparator queue macro
 * @param Address of device configuration structure
 * @param Change comparator queue on the fly flag
 * @note This function is not available for ADS1113
 */
void ads111x_comp_queue(uint8_t my_arg, ads111x_cfg_t* device_cfg, bool otf);

/*
 * Flash the ADS111x configuration register based on device configuration structure
 * @param I2C master device handle structure
 * @param Address of device configuration structure
 * @return 
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 */
esp_err_t initialize_ads111x(i2c_master_dev_handle_t ads111x_dev_handle, ads111x_cfg_t* device_cfg);

/*
 * Reads the ADS111x configuration register
 * @param Address of device configuration structure
 * @param Address of the 16 bit config register container (Provided by user)
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 * @note For debug purpose only. Line comment this function if you dont need it
 */
esp_err_t ads111x_read_config_reg(ads111x_cfg_t* device_cfg, uint16_t* output_data);

/*
 * Reads the conversion register to get the raw value
 * @param Address of device configuration structure
 * @param Address of the 16 bit raw value (Provided by user)
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 */
esp_err_t ads111x_measure_raw(ads111x_cfg_t* device_cfg, uint16_t* output_data);

/*
 * Reads the conversion register to get the voltage value in volts (single-ended mode)
 * @param Address of device configuration structure
 * @param Address of the measurement value in float data type (Provided by user)
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 */
esp_err_t ads111x_measure_voltage(ads111x_cfg_t* device_cfg, float* output_data);

/*
 * Changes the mux setting and reads the conversion register to get the raw value
 * @param Address of device configuration structure
 * @param Multiplexer configuration macro
 * @param Address of the 16 bit raw value (Provided by user)
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 * @note This function is not available for ADS1113 and ADS1114
 * @note This function works for single-ended and differential mode
 * @note This function changes the default mux setting
 */
esp_err_t ads111x_measure_raw_specific_channel(ads111x_cfg_t* device_cfg, uint8_t mux_setting, uint16_t* output_data);

/*
 * Changes the mux setting and reads the conversion register to get the voltage value
 * @param Address of device configuration structure
 * @param Multiplexer configuration macro
 * @param Address of the measurement value in float data type (Provided by user)
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 * @note This function is not available for ADS1113 and ADS1114
 * @note This function works for single-ended and differential mode
 * @note This function changes the default mux setting
 */
esp_err_t ads111x_measure_voltage_specific_channel(ads111x_cfg_t* device_cfg, uint8_t mux_setting, float* output_data);

/*
 * Reads the conversion register to get the raw value for all of the single-ended channels
 * @param Address of device configuration structure
 * @param Address of the array of four member 16 bit raw value (Provided by user)
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 * @note This function is not available for ADS1113 and ADS1114
 * @note This function only works for single-ended mode
 * @note This function changes the default mux setting 
 */
esp_err_t ads111x_measure_raw_sweep(ads111x_cfg_t* device_cfg, uint16_t* output_data);

/*
 * Reads the conversion register to get the voltage value for all of the single-ended channels
 * @param Address of device configuration structure
 * @param Address of the array of four member float values (Provided by user)
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 * @note This function is not available for ADS1113 and ADS1114
 * @note This function only works for single-ended mode
 * @note This function changes the default mux setting 
 */
esp_err_t ads111x_measure_voltage_sweep(ads111x_cfg_t* device_cfg, float* output_data);

/*
 * Write raw threshold value from the device configuration structure to low threshold and high threshold register
 * @param Address of device configuration structure
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 *     ESP_ERR_INVALID_ARG: Invalid args. (Valid args. low thresh < high thresh)
 * @note This function is not available for ADS1113
 * @note Before using this function, make sure you already have desired gain amplifier setting
 */
esp_err_t ads111x_set_threshold_raw(ads111x_cfg_t* device_cfg);

/*
 * Write float threshold value to low threshold and high threshold register
 * @param Address of device configuration structure
 * @param Address of float low threshold value
 * @param Address of float high threshold value 
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 *     ESP_ERR_INVALID_ARG: Invalid args. (Valid args. low thresh < high thresh)
 * @note This function is not available for ADS1113
 * @note Before using this function, make sure you already have desired gain amplifier setting
 * @note Conversion to 16 bits 2s complement handled by this library
 */
esp_err_t ads111x_set_threshold_voltage(ads111x_cfg_t* device_cfg, float* low_thresh, float* high_thresh);

/*
 * Resets low threshold and high threshold register
 * @param Address of device configuration structure
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 * @note This function is not available for ADS1113
 */
esp_err_t ads111x_reset_threshold(ads111x_cfg_t* device_cfg);

/*
 * Select alert/ready pin function (alert pin default)
 * @param Address of device configuration structure
 * @param Alert/ready configuration macro
 * @return
 *     ESP_OK: success
 *     ESP_ERR_TIMEOUT: I2C communication timeout or device not found
 * @note This function is not available for ADS1113
 */
esp_err_t ads111x_alert_ready_pin(bool my_arg, ads111x_cfg_t* device_cfg);

#endif // ADS111X_H