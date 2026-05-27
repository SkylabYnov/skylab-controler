// Aerisys controller firmware entry point.
//
// Wires together:
//   - EspNowLink         : ESP-NOW transport and peer persistence
//   - PairingManager     : bonding state machine on top of the link
//   - StatusLed          : visible link / pairing status on a single LED
//   - JoysticksManager   : two analog joysticks -> ControllerRequestDTO
//   - ButtonsManager     : declarative button table (press / long-press)
//   - PcSerialBridge     : optional HIL/test entry over UART (compile-time)

#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "config/Pins.h"
#include "config/Timings.h"

#include "core/EspNowLink.h"
#include "core/PairingManager.h"

#include "ui/StatusLed.h"

#include "input/Button.h"
#include "input/ButtonsManager.h"
#include "input/JoysticksManager.h"

#include "bridge/PcSerialBridge.h"

#include <ControllerRequestDTO.h>

namespace
{
    constexpr char TAG[] = "MAIN";

    // Set to true to disable the embedded UI (sticks + buttons) and drive
    // the controller from a host PC over UART instead. The two paths are
    // mutually exclusive because they both want UART0.
    constexpr bool MODE_COMPUTER = false;
}

using namespace Aerisys::Controller;

// ---------------------------------------------------------------------
// Shared toggle state for the arming / motor-state buttons.
// Stored as file-scope statics so the button lambdas can reference them
// safely after app_main() returns.
// ---------------------------------------------------------------------
namespace
{
    bool armingState     = false;
    bool motorStateValue = false;

    // Tiny task that mirrors the pairing state onto the status LED.
    // Kept here (vs. inside StatusLed) so the LED module stays free of
    // any business-logic dependency.
    [[noreturn]] void statusSupervisorTask(void *arg)
    {
        struct Ctx {
            PairingManager *pairing;
            StatusLed      *led;
        };
        Ctx *ctx = static_cast<Ctx*>(arg);

        StatusLed::Pattern lastPattern = StatusLed::Pattern::Off;
        while (true) {
            StatusLed::Pattern wanted = ctx->pairing->isPairing()
                                         ? StatusLed::Pattern::FastBlink
                                         : StatusLed::Pattern::Off;
            if (wanted != lastPattern) {
                ctx->led->setPattern(wanted);
                lastPattern = wanted;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

extern "C" void app_main()
{
    // Standard ESP-IDF system bring-up.
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // ------------------------------------------------------------------
    // Power-rail settling delay before Wi-Fi PA spike.
    //
    // esp_wifi_start() (called inside EspNowLink::init below) triggers
    // the Wi-Fi PHY calibration which pulls a ~200-500 mA current spike
    // for a few hundred microseconds. On marginal USB supplies / cheap
    // cables, this collapses VDD below the brownout threshold (2.43 V
    // default) and the chip resets in a loop.
    //
    // This delay lets the 5 V rail's bulk caps and the 3.3 V LDO fully
    // stabilise BEFORE the radio fires up — empirically resolves most
    // boot-loops without hardware changes. The peak itself is unchanged
    // (intrinsic to PHY calibration), but the supply is at its best
    // state when it has to deliver it.
    //
    // True fix is hardware: use a powered USB hub / wall adapter, add a
    // 470 µF cap between VIN and GND, or migrate to a DevKit with a
    // beefier LDO + larger input caps. See controller README for details.
    vTaskDelay(pdMS_TO_TICKS(500));

    // ------------------------------------------------------------------
    // Radio transport + pairing
    // ------------------------------------------------------------------
    auto *link = new EspNowLink();
    if (!link->init()) {
        ESP_LOGE(TAG, "ESP-NOW link init failed");
        return;
    }

    auto *pairing = new PairingManager(link);
    pairing->init();

    // ------------------------------------------------------------------
    // Status LED + supervisor that drives it from pairing state
    // ------------------------------------------------------------------
    auto *led = new StatusLed(Pins::LED_STATUS);
    led->init();
    led->start();

    struct SupervisorCtx { PairingManager *pairing; StatusLed *led; };
    auto *ctx = new SupervisorCtx{pairing, led};
    xTaskCreate(statusSupervisorTask, "statusSupervisor",
                2048, ctx, 1, nullptr);

    // ------------------------------------------------------------------
    // Periodic ping (keeps the drone-side link timer happy)
    // ------------------------------------------------------------------
    xTaskCreate([](void *arg) {
        auto *l = static_cast<EspNowLink*>(arg);
        while (true) {
            l->sendPing();
            vTaskDelay(pdMS_TO_TICKS(Timings::PING_INTERVAL_US / 1000));
        }
    }, "pingTask", 2048, link, 1, nullptr);

    if (MODE_COMPUTER) {
        // ------------------------------------------------------------------
        // PC bridge path (HIL / scripted tests)
        // ------------------------------------------------------------------
        auto *bridge = new PcSerialBridge(link, 115200);
        xTaskCreate([](void *arg) {
            static_cast<PcSerialBridge*>(arg)->task();
        }, "pcBridgeTask", 4096, bridge, 5, nullptr);
    } else {
        // ------------------------------------------------------------------
        // Embedded UI path: joysticks + physical buttons
        // ------------------------------------------------------------------
        auto *joysticks = new JoysticksManager(link);
        joysticks->init();

        std::vector<Button> buttonTable = {
            {
                // Arming uses a long-press toggle (Timings::ARMING_LONG_PRESS_MS)
                // so an accidental brush against the button mid-flight cannot
                // disarm the drone. Toggle is computed locally on the
                // controller and the wire payload is the *absolute* state
                // (drone receives the new value, not "flip whatever you have").
                .pin         = Pins::BTN_ARMING,
                .name        = "arming",
                .pullUp      = true,
                .longPressMs = Timings::ARMING_LONG_PRESS_MS,
                .onLongPress = [link]() {
                    armingState = !armingState;
                    // esp-lib v1.1.0+ POD API: value + has_X flag, no heap.
                    ControllerRequestDTO dto;
                    dto.buttonMotorArming     = armingState;   // absolute
                    dto.has_buttonMotorArming = true;
                    dto.initCounter();
                    link->sendControllerRequest(dto);
                    ESP_LOGI(TAG, "Arming -> %s", armingState ? "ARMED" : "DISARMED");
                },
            },
            {
                // Same safety rationale as arming: long-press toggle to
                // protect against accidental motor cuts in flight.
                .pin         = Pins::BTN_MOTOR_STATE,
                .name        = "motor_state",
                .pullUp      = true,
                .longPressMs = Timings::MOTOR_STATE_LONG_PRESS_MS,
                .onLongPress = [link]() {
                    motorStateValue = !motorStateValue;
                    // esp-lib v1.1.0+ POD API: value + has_X flag, no heap.
                    ControllerRequestDTO dto;
                    dto.buttonMotorState     = motorStateValue;   // absolute
                    dto.has_buttonMotorState = true;
                    dto.initCounter();
                    link->sendControllerRequest(dto);
                    ESP_LOGI(TAG, "Motor state -> %s", motorStateValue ? "ON" : "OFF");
                },
            },
            {
                .pin         = Pins::BTN_ASSOCIATION,
                .name        = "association",
                .pullUp      = true,
                .longPressMs = static_cast<int>(Timings::ASSOCIATION_LONG_PRESS_MS),
                .onLongPress = [pairing]() {
                    ESP_LOGW(TAG, "Long press on association: forgetting peer");
                    pairing->forgetPeer();
                },
            },
            // To add a new button: append one more entry here.
            // {
            //     .pin       = Pins::BTN_AUTOTUNE,
            //     .name      = "autotune",
            //     .onPressed = [link]() { /* ... */ },
            // },
        };
        auto *buttons = new ButtonsManager(std::move(buttonTable));
        buttons->init();

        xTaskCreate([](void *arg) {
            static_cast<JoysticksManager*>(arg)->task();
        }, "joysticksTask", 4096, joysticks, 5, nullptr);

        xTaskCreate([](void *arg) {
            static_cast<ButtonsManager*>(arg)->task();
        }, "buttonsTask", 4096, buttons, 5, nullptr);
    }

    ESP_LOGI(TAG, "Controller boot complete");
}
