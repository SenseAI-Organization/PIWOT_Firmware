#include <stdio.h>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include "esp_adc/adc_oneshot.h"
#include "sd_storage_sense.hpp"
#include "spi_gp_sense.hpp"
#include <sys/time.h>

// Adjust these according to your ESP32 chip
#define ADC_CHANNEL_1 ADC_CHANNEL_4  // GPIO15
#define ADC_CHANNEL_2 ADC_CHANNEL_5  // GPIO16
#define ADC_UNIT ADC_UNIT_2

constexpr gpio_num_t kCsPin = GPIO_NUM_10;
constexpr gpio_num_t kSclPin = GPIO_NUM_12;
constexpr gpio_num_t kMosiPin = GPIO_NUM_11;
constexpr gpio_num_t kMisoPin = GPIO_NUM_13;

bool logCSVLine(SD &sd, const std::string &folderName, const std::string &fileName, const std::string &line);


void setManualTime() //SET TIME
{
    struct tm t = {};
    t.tm_year = 2025 - 1900; // Year
    t.tm_mon  = 8;           // September (0 = Jan)
    t.tm_mday = 26;          // Day
    t.tm_hour = 10;
    t.tm_min  = 1;
    t.tm_sec  = 0;

    time_t now = mktime(&t);
    struct timeval tv = { .tv_sec = now };
    settimeofday(&tv, NULL);
}

extern "C" void app_main() { 
    setManualTime();
    //vTaskDelay(pdMS_TO_TICKS(10000));
    // 1. Initialize SPI bus and SD driver
    SPI spiMaster(SPI::SpiMode::kMaster, SPI2_HOST, kMosiPin, kMisoPin,
                  kSclPin);
    // To initialize spi bus
    esp_err_t err = spiMaster.init();
    if (err) {
        printf("SPI error while init: %s\n", esp_err_to_name(err));
    }

    SD sd(spiMaster, kCsPin);
    err = sd.init();

    if (err) {
        printf("SD error while init: %s\n", esp_err_to_name(err));
    } else {
        printf("SD was initialized!\n");
    }

    // 2. Mount SD card
    FRESULT error = sd.mountCard();
    if (err) {
        printf("SD error while mounting: %s\n", sd.getFastFsErrName(error));
    } else {
        std::string currentPath = sd.getCurrentDir();
        printf(
            ("Card mounted, here is the root path - " + currentPath).c_str());
        printf("\n");
    }

    // --- ADC Setup ---
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = ADC_UNIT;
    init_config.clk_src = ADC_RTC_CLK_SRC_DEFAULT;  // default clock source
    init_config.ulp_mode = ADC_ULP_MODE_DISABLE;    // disable ULP

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {};
    chan_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    chan_config.atten = ADC_ATTEN_DB_12;  // recommended instead of deprecated DB_11

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_1, &chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_2, &chan_config));

    const std::string folder = "logs";
    const std::string file   = "motorData2.csv";

    
    int sampleCounter = 0;
    
    while (true) {
        int raw1 = 0, raw2 = 0;

        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &raw1));
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_2, &raw2));

        // Timestamp
        time_t now;
        time(&now);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

        // CSV line: sample,timestamp,value1,value2
        std::stringstream line;
        line << sampleCounter++ << "," << time_str << "," << raw1 << "," << raw2;

        sd.mountCard();
        if (logCSVLine(sd, folder, file, line.str())) {
            printf("Logged: %s\n", line.str().c_str());
        } else {
            printf("Failed to log line\n");
        }
        sd.unmountCard();

        std::this_thread::sleep_for(std::chrono::seconds(40));
    }
}


bool logCSVLine(SD &sd, const std::string &folderName, const std::string &fileName, const std::string &line) {
    std::string finalLine = line;
    if (finalLine.empty() || finalLine.back() != '\n') {
        finalLine += "\n";
    }

    // Go to root first
    if (sd.goToDir("0:/") != FR_OK) {
        printf("CSV error: could not go to root dir\n");
        return false;
    }

    // Ensure folder exists
    if (sd.goToDir(folderName) != FR_OK) {
        if (sd.createDir(folderName) != FR_OK) {
            printf("CSV error: could not create folder '%s'\n", folderName.c_str());
            return false;
        }
        sd.goToDir(folderName);
    }

    // Create file if missing
    if (sd.openFile(fileName, SD::OpenMode::kOpenReadOnly) != FR_OK) {
        if (sd.createFile(fileName) != FR_OK) {
            printf("CSV error: could not create file '%s'\n", fileName.c_str());
            return false;
        } else {
            sd.openFile(fileName, SD::OpenMode::kOpenAppend);
            sd.fileWrite("sample,timestamp,value1,value2,value3,value4\n");
            sd.closeFile();
        }
    } else {
        sd.closeFile();
    }

    // Append line
    if (sd.openFile(fileName, SD::OpenMode::kOpenAppend) == FR_OK) {
        if (sd.fileWrite(finalLine) != FR_OK) {
            sd.closeFile();
            printf("CSV error: write failed\n");
            return false;
        }
        sd.closeFile();
        return true;
    }

    printf("CSV error: could not open for append\n");
    return false;
}