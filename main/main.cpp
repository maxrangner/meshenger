#include "application.h"
#include "app_log.h"

extern "C" void app_main(void)
{
    app::log_boot_banner();

    static app::Application app;

    app.init();
}
