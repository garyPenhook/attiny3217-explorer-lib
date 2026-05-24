# Board Map

Central aliases live in `include/board.hpp`.

| Alias | ATtiny3217 pin | Function |
| --- | --- | --- |
| `board::StatusLed` | `PB5` | Status LED output |
| `board::ButtonIn` | `PB4` | Active-low button input with pull-up and falling-edge interrupt |
| `board::Usart0Tx` | `PB2` | USART0 transmit |
| `board::Usart0Rx` | `PB3` | USART0 receive |
| `board::Spi0Sck` | `PA3` | SPI0 clock |
| `board::Spi0Mosi` | `PA4` | SPI0 MOSI |
| `board::Spi0Miso` | `PA5` | SPI0 MISO |
| `board::Twi0Scl` | `PB0` | TWI0/I2C clock |
| `board::Twi0Sda` | `PB1` | TWI0/I2C data |

`platform_init::run()` configures the default routes for USART0, SPI0, and TWI0 through PORTMUX and sets the direction registers for these board-level uses.

The current analog wrapper samples `ADC0 AIN7` and scales it as a 0-5000 mV reading using VDD as the ADC reference.
