// LibXR objects and hardware registration of the HPM5301EVKLite BSP.
// Pins and clocks are set up in main.cpp. Put application code between the
// "User Code Begin" and "User Code End" markers; the layout follows bsp_stm32f103.
#include "app_main.h"

#include <atomic>
#include <cstdint>

#include "board.h"
#include "hpm_gpio.hpp"
#include "hpm_i2c.hpp"
#include "hpm_interrupt.h"
#include "hpm_pwm.hpp"
#include "hpm_timebase.hpp"
#include "libxr.hpp"
#include "xrobot_main.hpp"

using namespace LibXR;

/* User Code Begin 1 */
// KEY interrupt counter, incremented in the GPIO interrupt callback.
static std::atomic<uint32_t> key_irq_count{0};

static void OnKeyInterrupt(bool in_isr, std::atomic<uint32_t>* count)
{
  (void)in_isr;
  count->fetch_add(1, std::memory_order_relaxed);
}
/* User Code End 1 */

// DMA buffers: HPMGPIO, HPMI2C and HPMPWM need none. The UART and SPI buffers are added
// together with their drivers.

// GPIO0 port A interrupt (KEY, PA03): hand the port over to LibXR.
SDK_DECLARE_EXT_ISR_M(BOARD_APP_GPIO_IRQ, key_gpio_isr)
void key_gpio_isr(void) { libxr_hpm_gpio_check_interrupt(BOARD_APP_GPIO_INDEX); }

extern "C" void app_main(void)
{
  /* User Code Begin 2 */
  /* User Code End 2 */

  // Timebase and platform
  static HPMTimebase timebase;
  PlatformInit();

  // GPIO
  static HPMGPIO LED(BOARD_LED_GPIO_CTRL, BOARD_LED_GPIO_INDEX, BOARD_LED_GPIO_PIN);
  static HPMGPIO KEY(BOARD_APP_GPIO_CTRL, BOARD_APP_GPIO_INDEX, BOARD_APP_GPIO_PIN,
                     BOARD_APP_GPIO_IRQ);
  // The pressed level decides the pull and the active edge.
  constexpr bool key_pressed_level = (BOARD_BUTTON_PRESSED_VALUE != 0u);
  KEY.SetConfig({key_pressed_level ? GPIO::Direction::RISING_INTERRUPT
                                   : GPIO::Direction::FALL_INTERRUPT,
                 key_pressed_level ? GPIO::Pull::DOWN : GPIO::Pull::UP});
  KEY.RegisterCallback(GPIO::Callback::Create(OnKeyInterrupt, &key_irq_count));
  KEY.EnableInterrupt();

  // PWM
  static HPMPWM pwm_gptmr0_ch1(reinterpret_cast<LibXRHpmPwmType*>(BOARD_GPTMR_PWM),
                               BOARD_GPTMR_PWM_CLK_NAME, BOARD_GPTMR_PWM_CHANNEL, 0,
                               false, false);
  pwm_gptmr0_ch1.SetConfig({1000});

  // I2C
  static HPMI2C i2c3(BOARD_APP_I2C_BASE, BOARD_APP_I2C_CLK_NAME, false);

  // Hardware registration
  XR_REGISTER(LED, LibXR::GPIO);
  XR_REGISTER(KEY, LibXR::GPIO);

  XR_REGISTER(pwm_gptmr0_ch1, LibXR::PWM);

  XR_REGISTER(i2c3, LibXR::I2C);

  /* User Code Begin 3 */
  /* User Code End 3 */
  XROBOT_MAIN();
}
