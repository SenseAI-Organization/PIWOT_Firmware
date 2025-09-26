#include "espnow_sense.hpp"
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_system.h"



#define TAG "ESP_NOW_CLIENT"
const char* devName = "DroneFB";
//48:ca:43:15:ff:2c
uint8_t serverMac[6] = {0x48, 0xCA, 0x43, 0x15, 0xFF, 0x2C};

extern "C" void app_main() { 
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
    int ledState=1;
    while (true) {
        if (espClient.hasNewMessage()) {
            //std::string receivedData;
            espClient.receiveData(receivedData);
            ESP_LOGI(TAG, "Received data: %s", receivedData.c_str());
        }
        if(receivedData == "IDLE"){
            ledState=1;
        }

        switch (ledState) {
            case 't':

                break;
            case 'r':

                break;
            case 'c':

                break;
            default:
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

bool sendEspNowData(EspNow &clientSender, std::string dataToSend){
    //const char* TAG_ = "send Function";
    clientSender.initialize();
    clientSender.setServer(serverMac);
    esp_err_t ret = clientSender.sendData(dataToSend);
    clientSender.disconnect();
    if (ret){
        printf("success sending!");
        return true;
    } else {
        printf("error sending!");
        return false;
    }
    
}