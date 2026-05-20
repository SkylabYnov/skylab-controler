#include "bridge/PcSerialBridge.h"

#include "core/EspNowLink.h"
#include "config/Limits.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#include <cstring>
#include <ControllerRequestDTO.h>

namespace
{
    constexpr char        TAG[]      = "PcSerialBridge";
    constexpr uart_port_t UART_NUM   = UART_NUM_0;
    constexpr int         BUF_SIZE   = 1024;
    constexpr uint8_t     SYNC_BYTE0 = 0xAA;
    constexpr uint8_t     SYNC_BYTE1 = 0x55;
    constexpr size_t      MAX_PAYLOAD = 64;
}

namespace Aerisys::Controller
{

PcSerialBridge::PcSerialBridge(EspNowLink *link, int baudRate)
    : link(link)
{
    uart_config_t cfg = {};
    cfg.baud_rate  = baudRate;
    cfg.data_bits  = UART_DATA_8_BITS;
    cfg.parity     = UART_PARITY_DISABLE;
    cfg.stop_bits  = UART_STOP_BITS_1;
    cfg.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;
    // rx_flow_ctrl_thresh + flags stay zero-initialised by the
    // value-init above, which keeps -Wmissing-field-initializers silent
    // across IDF versions that add new optional fields.
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &cfg));
    ESP_LOGI(TAG, "UART0 initialised at %d bauds", baudRate);
}

PcSerialBridge::~PcSerialBridge()
{
    uart_driver_delete(UART_NUM);
    ESP_LOGI(TAG, "UART0 driver uninstalled");
}

void PcSerialBridge::task()
{
    ESP_LOGI(TAG, "PcSerialBridge task running");

    uint8_t byte;
    uint8_t payload[MAX_PAYLOAD];

    while (true) {
        // ---- 1. sync header ----
        uart_read_bytes(UART_NUM, &byte, 1, portMAX_DELAY);
        if (byte != SYNC_BYTE0) continue;

        uart_read_bytes(UART_NUM, &byte, 1, portMAX_DELAY);
        if (byte != SYNC_BYTE1) continue;

        // ---- 2. length ----
        uint8_t length;
        uart_read_bytes(UART_NUM, &length, 1, portMAX_DELAY);
        if (length > MAX_PAYLOAD) {
            ESP_LOGW(TAG, "Frame too large (%u), dropping", length);
            continue;
        }

        // ---- 3. payload + checksum ----
        uart_read_bytes(UART_NUM, payload, length, portMAX_DELAY);
        uint8_t checksum;
        uart_read_bytes(UART_NUM, &checksum, 1, portMAX_DELAY);

        if (computeChecksum(payload, length) != checksum) {
            ESP_LOGW(TAG, "Bad checksum, dropping frame");
            continue;
        }

        if (length < sizeof(ControllerPacket)) {
            ESP_LOGW(TAG, "Payload too small for ControllerPacket (%u)", length);
            continue;
        }

        ControllerPacket pkt;
        std::memcpy(&pkt, payload, sizeof(pkt));
        dispatchPacket(pkt);
    }
}

void PcSerialBridge::dispatchPacket(const ControllerPacket &pkt)
{
    // Convert host-side normalised sticks ([-1, +1]) to internal raw range.
    const auto toRaw = [](float n) -> int16_t {
        return static_cast<int16_t>(((n + 1.0f) * 0.5f) * JoystickModel::JOYSTICK_MAX);
    };

    JoystickModel left (toRaw(pkt.LeftStickX),  toRaw(pkt.LeftStickY));
    JoystickModel right(toRaw(pkt.RightStickX), toRaw(pkt.RightStickY));

    if (lastLeft != left || lastRight != right) {
        ControllerRequestDTO dto;
        dto.ConvertJoyStickToFlightController(left, right);
        dto.initCounter();

        if (Limits::JOYSTICK_THROTTLE_LIMIT >= 0.0f && dto.flightController) {
            dto.flightController->throttle *= Limits::JOYSTICK_THROTTLE_LIMIT;
        }

        link->sendControllerRequest(dto);
        lastLeft  = left;
        lastRight = right;
    }

    if (pkt.motorState != lastMotorState) {
        lastMotorState = pkt.motorState;
        ControllerRequestDTO dto;
        dto.buttonMotorState = new bool(pkt.motorState);
        dto.initCounter();
        link->sendControllerRequest(dto);
    }

    if (pkt.motorArming != lastMotorArming) {
        lastMotorArming = pkt.motorArming;
        ControllerRequestDTO dto;
        dto.buttonMotorArming = new bool(pkt.motorArming);
        dto.initCounter();
        link->sendControllerRequest(dto);
    }
}

uint8_t PcSerialBridge::computeChecksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += data[i];
    }
    return sum & 0xFF;
}

} // namespace Aerisys::Controller
