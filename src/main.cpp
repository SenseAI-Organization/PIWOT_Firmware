#include "espnow_sense.hpp"
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_system.h"
#include <driver/gpio.h>

#define TAG "ESP_NOW_CLIENT"

constexpr gpio_num_t LED_PIN = GPIO_NUM_13;  // Replace with your actual pin
constexpr gpio_num_t ACTUATOR_PIN = GPIO_NUM_27;

const char* devName = "DroneFB";
//48:ca:43:15:ff:2c
uint8_t serverMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void led_ONOFF(){
    gpio_set_level(LED_PIN,1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(LED_PIN,0);
}
void actuator_ONOFF(){
    gpio_set_level(ACTUATOR_PIN,1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(ACTUATOR_PIN,0);
}
void led_ON(){
    gpio_set_level(LED_PIN,1);
}
void led_blink(int nTimes){
    for(int i=0;i<nTimes;i++){
        gpio_set_level(LED_PIN,1);
        vTaskDelay(pdMS_TO_TICKS(30));
        gpio_set_level(LED_PIN,0);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

extern "C" void app_main() { 
    // Configure both pins as outputs
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_PIN) | (1ULL << ACTUATOR_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN,0);
    gpio_set_level(ACTUATOR_PIN,0);
    EspNow espClient(6, 1);
    bool sendEspNowData(EspNow &clientSender, std::string dataToSend);


    int64_t start_time = esp_timer_get_time();

    // Initialize and set server
    ESP_LOGI(TAG, "Initializing ESP-NOW client");
    espClient.initialize();
    ESP_LOGI(TAG, "+++++++++ Init: %lld ms", (esp_timer_get_time() - start_time) / 1000);

    // Set the server/receiver MAC address
    espClient.setServer(serverMac);
    ESP_LOGI(TAG, "+++++++++ Server MAC set: %lld ms", (esp_timer_get_time() - start_time) / 1000);

    // Send a simple string message
    char Message[32];
    snprintf(Message, sizeof(Message),"Hello from %s",devName);

    espClient.sendData(std::string(Message));
    std::string receivedData= "IDLE";
    int state=1;
    uint32_t currentMillis=0;
    uint32_t startMillis=0;
    while (true) {
        currentMillis= esp_timer_get_time() / 1000;

        if (espClient.hasNewMessage()) {
            receivedData.clear();
            espClient.receiveData(receivedData);
            ESP_LOGI(TAG, "Received data: %s", receivedData.c_str());
        }

        if (receivedData.find("IDLE") != std::string::npos) {
            printf("IDLE\n");
            state = 1;
        } else if (receivedData.find("START") != std::string::npos) {
            printf("START\n");
            startMillis = currentMillis;
            state = 2;
        } else if (receivedData.find("RELEASE") != std::string::npos) {
            printf("RELEASE\n");
            state = 3;
        }


        if(state==2 && (currentMillis-startMillis > 60000*3)) state=3;

        switch (state) {
            case 1:
                led_ONOFF();
                break;
            case 2:
                led_ON();
                receivedData.clear();
                break;
            case 3:
                led_blink(20);
                actuator_ONOFF();
                state=1;
                receivedData= " ";
                break;
            default:
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        //if(currentMillis > 60000) receivedData= "START";
    }
}

bool sendEspNowData(EspNow &clientSender, std::string dataToSend){
    //const char* TAG_ = "send Function";
    clientSender.initialize();
    clientSender.setServer(serverMac);
    esp_err_t ret = clientSender.sendData(dataToSend);
    clientSender.disconnect();
    if (ret){
        printf("success sending!\n");
        return true;
    } else {
        printf("error sending!\n");
        return false;
    }
    
}