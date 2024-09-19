#include "iot_qbus_port.h"
#include "esp_log.h"

extern qBSlave_st qbusSlave;

static QueueHandle_t uart_queue;

static void IRAM_ATTR uart_event_task(void *pvParameters)
{
    
    uart_event_t event;
    uint8_t* data = (uint8_t*) malloc(UART_QBUS_RX_SIZE);
    for (;;) {
        // Waiting for UART event.
        if (xQueueReceive(uart_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            switch (event.type) {
            case UART_DATA:
                // Reading the received data.
                uart_read_bytes(UART_PORT_NUM, data, event.size, portMAX_DELAY);
                for (int i = 0; i < event.size; i++) {
                    byteReceived(data[i]);
                }
                break;
            case UART_FIFO_OVF:
                ESP_LOGI("UART", "hw fifo overflow");
                uart_flush_input(UART_PORT_NUM);
                xQueueReset(uart_queue);
                break;
            case UART_BUFFER_FULL:
                    ESP_LOGI("UART", "ring buffer full");
                uart_flush_input(UART_PORT_NUM);
                xQueueReset(uart_queue);
                break;
            case UART_PARITY_ERR:
                ESP_LOGI("UART", "uart parity error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGI("UART", "uart frame error");
                break;
            default:
                ESP_LOGI("UART", "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(data);
    vTaskDelete(NULL);
}

void IotQbusUartPortInit(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Install UART driver using an event queue to handle UART events.
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_QBUS_RX_SIZE , UART_QBUS_TX_SIZE, 30, &uart_queue, 0));

    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);

}

void IotQbusSendByte(uint8_t sendDataQbus)
{
    uart_write_bytes(UART_PORT_NUM, (char*)&sendDataQbus, 1);
}

void byteReceived(uint8_t recvChar)
{
    QBSlavePortByteReceived(&qbusSlave, recvChar);
}

void IotQbusInterruptEnable(uint8_t xTxEnable, uint8_t xRxEnable)
{
    // Interrupts are automatically managed by the driver.
}
