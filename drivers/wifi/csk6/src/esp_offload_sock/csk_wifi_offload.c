#include "csk6/priv/csk_wifi_priv.h"
#include "ec718_conn.h"

int csk_wifi_init(void)
{
    esp_conn_init();
    return 0;
}
