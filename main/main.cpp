#include "application.h"
#include "esp_log.h"

static const char *TAG = "main";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Booting up...");

    static app::Application app;

    app.init();
}
