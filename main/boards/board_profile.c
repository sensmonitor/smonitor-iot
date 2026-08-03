#include "boards/board_profile.h"

#include "boards/generic_esp32_uart.h"
#include "boards/lilygo_t_sim7000g.h"
#include "boards/lilygo_t_sim7080_s3.h"
#include "sdkconfig.h"

smonitor_modem_config_t smonitor_board_modem_config(void)
{
#if CONFIG_SMONITOR_BOARD_GENERIC_ESP32_UART
    return smonitor_generic_esp32_uart_modem_config();
#elif CONFIG_SMONITOR_BOARD_LILYGO_T_SIM7080_S3
    return smonitor_lilygo_t_sim7080_s3_modem_config();
#else
    return smonitor_lilygo_t_sim7000g_modem_config();
#endif
}
