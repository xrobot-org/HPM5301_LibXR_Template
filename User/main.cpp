// Board bring-up of the HPM5301EVKLite BSP.
// Sets up the SDK board, then the pins and clocks of every peripheral the application
// uses, with the SDK board helpers of boards/hpm5301evklite. The LibXR objects for these
// peripherals are created in app_main().
#include <cstdlib>

#include "app_main.h"
#include "board.h"

// libstdc++'s default terminate handler links the C++ demangler (about 20 KB of code),
// too much for the 128 KB ILM of the debug-ram preset. LibXR does not use exceptions.
namespace __gnu_cxx
{
void __verbose_terminate_handler() { std::abort(); }
}  // namespace __gnu_cxx

int main(void)
{
  // Clocks, console on UART0 (PA00 TX / PA01 RX), USB PHY, banner
  board_init();

  // GPIO: LED PA10 (active low, output, off) and button PA03 (input, pull-down)
  board_init_led_pins();
  board_init_gpio_pins();

  // ADC16: ADC0 on the AHB clock source.
  // The SDK pin helper puts PB08-PB15 into analog mode. The peripherals below use some of
  // these pads with a digital function, and each of them rewrites its pads afterwards, so
  // keep this call first. Analog pads left: PB10 (ADC0.2, BOARD_APP_ADC16_CH_1) and PB11.
  board_init_adc16_pins();
  board_init_adc_clock(BOARD_APP_ADC16_BASE, true);

  // ACMP: PB09 comparator output, PB11 analog input (CMP1_INN4)
  board_init_acmp_pins();
  board_init_acmp_clock(BOARD_ACMP);

  // GPTMR0 PWM: PB08 (GPTMR0_COMP_1)
  board_init_gptmr_clock(BOARD_GPTMR_PWM);
  board_init_gptmr_channel_pin(BOARD_GPTMR_PWM, BOARD_GPTMR_PWM_CHANNEL, true);

  // I2C3: PB13 SCL / PB12 SDA
  board_init_i2c_clock(BOARD_APP_I2C_BASE);
  init_i2c_pins(BOARD_APP_I2C_BASE);

  // UART3: PB15 TX / PB14 RX (UART0 is the console, set up by board_init())
  board_init_uart(BOARD_APP_UART_BASE);

  // SPI1: PA26 CS / PA27 SCLK / PA28 MISO / PA29 MOSI
  board_init_spi_pins(BOARD_APP_SPI_BASE);
  board_init_spi_clock(BOARD_APP_SPI_BASE);

  // USB0: PA24 D+ / PA25 D- (analog), PY00 ID, PY01 OC
  board_init_usb(HPM_USB0);

  app_main();
  return 0;
}
