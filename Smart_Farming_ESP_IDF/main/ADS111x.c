#include <string.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ADS111x.h"

static const char *TAG = "ADS111x: ";

static int i2c_timeout_ms = 100; // I2C timeout in milliseconds

static void ads111x_split_16bit(uint16_t input_val, uint8_t* output_MSB, uint8_t* output_LSB)
{
    *output_MSB = (uint8_t) (input_val >> 8);
    *output_LSB = (uint8_t) (input_val & 0x00FF);
}

static void ads111x_write_to_reg(ads111x_cfg_t* device_cfg, uint8_t device_reg)
{
    device_cfg->buffer[0] = device_reg;

    uint8_t MSB_data = 0;
    uint8_t LSB_data = 0;

    if (device_reg == ADS111x_CFG_REG)
    {    
        device_cfg->buffer[1] = device_cfg->MSB_config_data;
        device_cfg->buffer[2] = device_cfg->LSB_config_data;
    }

    else if (device_reg == ADS111x_HI_THRESH_REG)
    {
        ads111x_split_16bit(device_cfg->high_threshold, &MSB_data, &LSB_data);
        device_cfg->buffer[1] = MSB_data;
        device_cfg->buffer[2] = LSB_data;
    }

    else if (device_reg == ADS111x_LO_THRESH_REG)
    {
        ads111x_split_16bit(device_cfg->low_threshold, &MSB_data, &LSB_data);
        device_cfg->buffer[1] = MSB_data;
        device_cfg->buffer[2] = LSB_data;
    }

    device_cfg->ADS111x_err = i2c_master_transmit(device_cfg->ads111x_i2c_dev_handle, (uint8_t*) device_cfg->buffer, 3, i2c_timeout_ms);
}

void ads111x_read_from_reg(ads111x_cfg_t* device_cfg, uint8_t device_reg)
{
    // Send register address to ADS111x
    device_cfg->buffer[0] = device_reg;
    device_cfg->ADS111x_err = i2c_master_transmit(device_cfg->ads111x_i2c_dev_handle, (uint8_t*) device_cfg->buffer, 1, i2c_timeout_ms);
    if (device_cfg->ADS111x_err != ESP_OK) 
        ESP_LOGE(TAG, "Error send address of register: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);

    // Receive register value and put it inside the device_cfg->buffer[0] and device_cfg->buffer[1]
    device_cfg->ADS111x_err = i2c_master_receive(device_cfg->ads111x_i2c_dev_handle, (uint8_t*) device_cfg->buffer, 2, i2c_timeout_ms);
    if (device_cfg->ADS111x_err != ESP_OK) 
        ESP_LOGE(TAG, "Error receive register value: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
}

void ads111x_reset_config_reg(ads111x_cfg_t* device_cfg)
{
    memset(device_cfg, 0, sizeof(device_cfg));
    device_cfg->PGA_float = 2.048;
    device_cfg->MSB_config_data = 0x85;
    device_cfg->LSB_config_data = 0x83;
    device_cfg->ADS111x_err = ESP_OK;

    device_cfg->mux_config = 0x00;
    device_cfg->gain_amp = 0x02;
    device_cfg->operating_mode = 0x01;
    device_cfg->data_rate = 0x04;
    device_cfg->comp_mode = 0x00;
    device_cfg->comp_pol = 0x00;
    device_cfg->comp_latch = 0x00;
    device_cfg->comp_queue = 0x03;

    device_cfg->low_threshold = 0x8000;
    device_cfg->high_threshold = 0x7FFF;
}

esp_err_t ads111x_configure_address(ads111x_address_e addr, ads111x_cfg_t* device_cfg)
{
    if (addr == ADDRPIN_TO_GND)
        device_cfg->device_addr = 0x48;
    else if (addr == ADDRPIN_TO_VCC)
        device_cfg->device_addr = 0x49;
    else if (addr == ADDRPIN_TO_SDA)
        device_cfg->device_addr = 0x4A;
    else if (addr == ADDRPIN_TO_SCL)
        device_cfg->device_addr = 0x4B;
    else return ESP_ERR_INVALID_ARG;

    return ESP_OK;
}

void ads111x_mux_config(uint8_t my_arg, ads111x_cfg_t* device_cfg, bool otf)
{
    // Reset mux config field in MSB_config_data
    device_cfg->MSB_config_data &= 0x8F;

    // Set the new mux config value
    device_cfg->MSB_config_data |= (my_arg << 4);
    
    if (otf)
    {
        device_cfg->mux_config = my_arg;

        // Flash the config register
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
            ESP_LOGE(TAG, "Error configuring mux: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "MSB_config_data MUX: %d", device_cfg->MSB_config_data);
    #endif
}

void ads111x_gain_amp(uint8_t my_arg, ads111x_cfg_t* device_cfg, bool otf)
{
    // Reset prog. gain amp. field in MSB_config_data
    device_cfg->MSB_config_data &= 0xF1;

    // Set the new prog. gain amp. config value
    device_cfg->MSB_config_data |= (my_arg << 1);
 
    if (my_arg == ADS111x_FSR_6V144)
        device_cfg->PGA_float = 6.144;
    else if (my_arg == ADS111x_FSR_4V096)
        device_cfg->PGA_float = 4.096;
    else if (my_arg == ADS111x_FSR_2V048)
        device_cfg->PGA_float = 2.048;
    else if (my_arg == ADS111x_FSR_1V024)
        device_cfg->PGA_float = 1.024;
    else if (my_arg == ADS111x_FSR_0V512)
        device_cfg->PGA_float = 0.512;
    else device_cfg->PGA_float = 0.256;

    if (otf)
    {
        device_cfg->gain_amp = my_arg;

        // Flash the config register
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
            ESP_LOGE(TAG, "Error configuring gain amp.: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "MSB_config_data PGA: %d", device_cfg->MSB_config_data);
    #endif
}

void ads111x_operating_mode(bool my_arg, ads111x_cfg_t* device_cfg, bool otf)
{
    // Reset device operating mode field in MSB_config_data
    device_cfg->MSB_config_data &= 0xFE;

    if(my_arg == true)
        device_cfg->MSB_config_data |= 1;

    if (otf)
    {
        device_cfg->operating_mode = my_arg;

        // Flash the config register
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
            ESP_LOGE(TAG, "Error configuring operating mode: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "MSB_config_data MODE: %d", device_cfg->MSB_config_data);
    #endif
}

void ads111x_data_rate(uint8_t my_arg, ads111x_cfg_t* device_cfg, bool otf)
{
    // Reset data rate field in LSB_config_data
    device_cfg->LSB_config_data &= 0x1F;

    // Set the new data rate value 
    device_cfg->LSB_config_data |= (my_arg << 5);

    if (otf)
    {
        device_cfg->data_rate = my_arg;

        // Flash the config register
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
            ESP_LOGE(TAG, "Error configuring data rate: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "LSB_config_data DR: %d", device_cfg->LSB_config_data);
    #endif
}

void ads111x_comp_mode(bool my_arg, ads111x_cfg_t* device_cfg, bool otf)
{
    // Reset comp. mode field in LSB_config_data
    device_cfg->LSB_config_data &= 0xEF;

    if(my_arg == true)
        device_cfg->LSB_config_data |= (1 << 4);
    
    if (otf)
    {
        device_cfg->comp_mode = my_arg;

        // Flash the config register
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
            ESP_LOGE(TAG, "Error configuring comp. mode: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "LSB_config_data COMP_MODE: %d", device_cfg->LSB_config_data);
    #endif
}

void ads111x_comp_polarity(bool my_arg, ads111x_cfg_t* device_cfg, bool otf)
{
    // Reset comp. polarity field in LSB_config_data
    device_cfg->LSB_config_data &= 0xF7;

    if(my_arg == true)
        device_cfg->LSB_config_data |= (1 << 3);
    
    if (otf)
    {
        device_cfg->comp_pol = my_arg;

        // Flash the config register
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
            ESP_LOGE(TAG, "Error configuring comp. pol.: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "LSB_config_data COMP_POL: %d", device_cfg->LSB_config_data);
    #endif
}

void ads111x_comp_latch(bool my_arg, ads111x_cfg_t* device_cfg, bool otf)
{
    // Reset comp. latching field in ADS111x config register
    device_cfg->LSB_config_data &= 0xFB;

    if(my_arg == true)
        device_cfg->LSB_config_data |= (1 << 2);
    
    if (otf)
    {
        device_cfg->comp_latch = my_arg;

        // Flash the config register
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
            ESP_LOGE(TAG, "Error configuring comp. latching: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);    
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "LSB_config_data COMP_LAT: %d", device_cfg->LSB_config_data);
    #endif
}

void ads111x_comp_queue(uint8_t my_arg, ads111x_cfg_t* device_cfg, bool otf)
{
    // Reset comp. queue field in ADS111x config register
    device_cfg->LSB_config_data &= 0xFC;

    // Set the new comp. queue value
    device_cfg->LSB_config_data |= my_arg;
    
    if (otf)
    {
        device_cfg->comp_queue = my_arg;

        // Flash the config register
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
            ESP_LOGE(TAG, "Error configuring comp. queue: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "LSB_config_data COMP_QUE: %d", device_cfg->LSB_config_data);
    #endif
}

esp_err_t initialize_ads111x(i2c_master_dev_handle_t ads111x_dev_handle, ads111x_cfg_t* device_cfg)
{
    // Put the i2c_master_dev_handle_t value to the device configuration structure
    device_cfg->ads111x_i2c_dev_handle = ads111x_dev_handle;

    // Flash high threshold register
    ads111x_write_to_reg(device_cfg, ADS111x_HI_THRESH_REG);
    if (device_cfg->ADS111x_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error flashing high threshold register: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }
    
    // Flash low threshold register
    ads111x_write_to_reg(device_cfg, ADS111x_LO_THRESH_REG);
    if (device_cfg->ADS111x_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error flashing low threshold register: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    // Apply user settings
    ads111x_mux_config(device_cfg->mux_config, device_cfg, false);
    ads111x_gain_amp(device_cfg->gain_amp, device_cfg, false);
    ads111x_operating_mode(device_cfg->operating_mode, device_cfg, false);
    ads111x_data_rate(device_cfg->data_rate, device_cfg, false);
    ads111x_comp_mode(device_cfg->comp_mode, device_cfg, false);
    ads111x_comp_polarity(device_cfg->comp_pol, device_cfg, false);
    ads111x_comp_latch(device_cfg->comp_latch, device_cfg, false);
    ads111x_comp_queue(device_cfg->comp_queue, device_cfg, false);

    // Send the configuration register pointer address to ADS111x
    // Send the configuration register value
    ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
    if (device_cfg->ADS111x_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error configuring device: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

// For debug purpose only
// Comment this if you dont need it
esp_err_t ads111x_read_config_reg(ads111x_cfg_t* device_cfg, uint16_t* output_data)
{
    // Send config register address to ADS111x
    ads111x_read_from_reg(device_cfg, ADS111x_CFG_REG);
    if (device_cfg->ADS111x_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error reading register: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    uint16_t raw_data = device_cfg->buffer[0] << 8;
    raw_data |= device_cfg->buffer[1];
    *output_data = raw_data;
    
    #ifdef DEBUG
        ESP_LOGI(TAG, "%d %d", device_cfg->buffer[0], device_cfg->buffer[1]);
    #endif
    
    return ESP_OK;
}

esp_err_t ads111x_measure_raw(ads111x_cfg_t* device_cfg, uint16_t* output_data)
{
    if(device_cfg->operating_mode == ADS111x_SINGLE_SHOT)
    {
        // Trigger one measurement by setting OS bit field in config register
        device_cfg->MSB_config_data |= (1 << 7); 

        // Send the configuration register pointer address to ADS111x
        // Send the configuration register value
        ads111x_write_to_reg(device_cfg, ADS111x_CFG_REG);
        if (device_cfg->ADS111x_err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Error triggering measurement: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }
    }

    // Wait 150 ms
    vTaskDelay(pdMS_TO_TICKS(150));

    // Send conversion register address to ADS111x
    ads111x_read_from_reg(device_cfg, ADS111x_CONV_REG);
    if (device_cfg->ADS111x_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error receive conversion value: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    uint16_t raw_data = device_cfg->buffer[0] << 8;
    raw_data |= device_cfg->buffer[1];
    *output_data = raw_data;
    
    #ifdef DEBUG
        ESP_LOGI(TAG, "ADC Raw: %d %d", device_cfg->buffer[0], device_cfg->buffer[1]);
    #endif
    
    return ESP_OK;
}

esp_err_t ads111x_measure_voltage(ads111x_cfg_t* device_cfg, float* output_data)
{
    uint16_t output_raw = 0;
    device_cfg->ADS111x_err = ads111x_measure_raw(device_cfg, &output_raw);
    if (device_cfg->ADS111x_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error read conversion register: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    float adc_voltage = 0.0;
    // Differential mode
    if (device_cfg->mux_config < 0x04)
    {
        // Based on the datasheet, if output raw > 32767 it means negative voltage detected (differential mode)
        if (output_raw > 32767)
        {
            // We need to flip the bits
            output_raw ^= 0xFFFF;
            adc_voltage = ((float) output_raw / 32767.0f) * device_cfg->PGA_float * -1;
        }
        else 
            adc_voltage =  ((float) output_raw / 32767.0f) * device_cfg->PGA_float;
    }
    // Single-ended mode
    else
    {
        // Based on the testing, ADS111x can give raw value of 65535 when reading GND terminal (single-ended mode)
        // Datasheet said that too
        if (output_raw > 32767)
            output_raw = 0;

        adc_voltage =  ((float) output_raw / 32767.0f) * device_cfg->PGA_float;
    }

    #ifdef DEBUG
        ESP_LOGI(TAG, "output_raw: %d", output_raw);
        ESP_LOGI(TAG, "my_gain_amp: %f", device_cfg->PGA_float);
        ESP_LOGI(TAG, "adc_voltage: %f", adc_voltage);
    #endif

    *output_data = adc_voltage;

    return ESP_OK;
}

esp_err_t ads111x_measure_raw_specific_channel(ads111x_cfg_t* device_cfg, uint8_t mux_setting, uint16_t* output_data)
{
    // Change multiplexer setting on the fly
    ads111x_mux_config(mux_setting, device_cfg, true);

    // Measure raw value
    device_cfg->ADS111x_err = ads111x_measure_raw(device_cfg, output_data);
    if (device_cfg->ADS111x_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error measure raw specific channel: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t ads111x_measure_voltage_specific_channel(ads111x_cfg_t* device_cfg, uint8_t mux_setting, float* output_data)
{
    // Change multiplexer setting on the fly
    ads111x_mux_config(mux_setting, device_cfg, true);

    // Measure voltage
    device_cfg->ADS111x_err = ads111x_measure_voltage(device_cfg, output_data);

    if (device_cfg->ADS111x_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error measure voltage specific channel: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t ads111x_measure_raw_sweep(ads111x_cfg_t* device_cfg, uint16_t* output_data)
{
    // Change mux setting, starting from channel 0
    uint8_t my_mux_setting = 0x04;

    for (int i = 0; i < 4; i++)
    {
        device_cfg->ADS111x_err = ads111x_measure_raw_specific_channel(device_cfg, my_mux_setting, output_data);
        if (device_cfg->ADS111x_err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Error measure raw channel %d: %s (0x%x)", i, esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }

        output_data++;
        my_mux_setting += 1;
    }

    return ESP_OK;
}

esp_err_t ads111x_measure_voltage_sweep(ads111x_cfg_t* device_cfg, float* output_data)
{
    // Change mux setting, starting from channel 0
    uint8_t my_mux_setting = 0x04;

    for (int i = 0; i < 4; i++)
    {
        device_cfg->ADS111x_err = ads111x_measure_voltage_specific_channel(device_cfg, my_mux_setting, output_data);
        if (device_cfg->ADS111x_err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Error measure voltage channel %d: %s (0x%x)", i, esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }

        output_data++;
        my_mux_setting += 1;
    }

    return ESP_OK;
}

esp_err_t ads111x_set_threshold_raw(ads111x_cfg_t* device_cfg)
{
    // Invalid args. check
    if (device_cfg->low_threshold >= device_cfg->high_threshold)
        return ESP_ERR_INVALID_ARG;
    
    // Save previous comparator setting
    uint8_t prev_comp_set = device_cfg->comp_queue;

    // Turn off comparator
    if (device_cfg->comp_queue != ADS111x_COMP_QUEUE_DISABLE)
    {
        ads111x_comp_queue(ADS111x_COMP_QUEUE_DISABLE, device_cfg, true);
        if (device_cfg->ADS111x_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error turning off comparator: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }
    }

    // Send high threshold value
    ads111x_write_to_reg(device_cfg, ADS111x_HI_THRESH_REG);
    if (device_cfg->ADS111x_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error flashing high threshold register: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }
    
    // Send low threshold value
    ads111x_write_to_reg(device_cfg, ADS111x_LO_THRESH_REG);
    if (device_cfg->ADS111x_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error flashing low threshold register: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    // Turn comparator back on
    if (device_cfg->comp_queue != ADS111x_COMP_QUEUE_DISABLE)
    {
        ads111x_comp_queue(prev_comp_set, device_cfg, true);
        if (device_cfg->ADS111x_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error turning on comparator: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }
    }

    return ESP_OK;
}

static void ads111x_convert_voltage_to_raw(ads111x_cfg_t* device_cfg, float adc_voltage, uint16_t* adc_raw)
{
    if (adc_voltage >= 0)
        *adc_raw = (uint16_t) ((adc_voltage * 32767) / device_cfg->PGA_float);

    else
    {
        // Make it positive
        adc_voltage *= -1;

        *adc_raw = (uint16_t) ((adc_voltage * 32767) / device_cfg->PGA_float);

        // Flip the bits
        *adc_raw ^= 0xFFFF;
    }
}

esp_err_t ads111x_set_threshold_voltage(ads111x_cfg_t* device_cfg, float* low_thresh, float* high_thresh)
{
    // Invalid args. check
    if (*low_thresh >= *high_thresh)
        return ESP_ERR_INVALID_ARG;

    // Convert voltage to ADC raw value
    uint16_t raw_thresh = 0;
    ads111x_convert_voltage_to_raw(device_cfg, *high_thresh, &raw_thresh);
    device_cfg->high_threshold = raw_thresh;

    ads111x_convert_voltage_to_raw(device_cfg, *low_thresh, &raw_thresh);
    device_cfg->low_threshold = raw_thresh;

    device_cfg->ADS111x_err = ads111x_set_threshold_raw(device_cfg);
    if (device_cfg->ADS111x_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error set threshold voltage: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t ads111x_reset_threshold(ads111x_cfg_t* device_cfg)
{
    // Turn off comparator
    if (device_cfg->comp_queue != ADS111x_COMP_QUEUE_DISABLE)
    {
        ads111x_comp_queue(ADS111x_COMP_QUEUE_DISABLE, device_cfg, true);
        if (device_cfg->ADS111x_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error turning off comparator: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }
    }
    
    // Default high threshold
    device_cfg->high_threshold = 0x7FFF;

    ads111x_write_to_reg(device_cfg, ADS111x_HI_THRESH_REG);
    if (device_cfg->ADS111x_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error set default high threshold: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    // Default low threshold
    device_cfg->low_threshold = 0x8000;
    
    ads111x_write_to_reg(device_cfg, ADS111x_LO_THRESH_REG);
    if (device_cfg->ADS111x_err != ESP_OK)
    {
        ESP_LOGE(TAG, "Error set default low threshold: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t ads111x_alert_ready_pin(bool my_arg, ads111x_cfg_t* device_cfg)
{
    if (my_arg == ADS111x_PIN_CONV_READY)
    {
        // Turn off comparator
        ads111x_comp_queue(ADS111x_COMP_QUEUE_DISABLE, device_cfg, true);
        if (device_cfg->ADS111x_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error turning off comparator: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }

        // MSB high 1b
        device_cfg->high_threshold = 0x100;
        // MSB low 0b
        device_cfg->low_threshold = 0x00;

        device_cfg->ADS111x_err = ads111x_set_threshold_raw(device_cfg);
        if (device_cfg->ADS111x_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error flashing threshold for conv. ready pin: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }
    }
    
    else 
    {
        device_cfg->ADS111x_err = ads111x_reset_threshold(device_cfg);
        if (device_cfg->ADS111x_err != ESP_OK)
        {
            ESP_LOGE(TAG, "Error flashing threshold for alert pin: %s (0x%x)", esp_err_to_name(device_cfg->ADS111x_err), device_cfg->ADS111x_err);
            return ESP_ERR_TIMEOUT;
        }
    }

    return ESP_OK;
}