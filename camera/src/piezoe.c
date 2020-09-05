#include "soundcam.h"

/* Camera shutter
 * when you detect shutter button event, then send a GPIO signal to other GPIO pin wired PIEZO BUZZER */

#define JUST_A_LITTLE_BIT	500000

void shutter (void)
{
	peripheral_gpio_h gpio = NULL;

    peripheral_gpio_open(GPIO_PIEZOE, &gpio);
	peripheral_gpio_set_direction(gpio, PERIPHERAL_GPIO_DIRECTION_OUT_INITIALLY_LOW);

	peripheral_gpio_write (gpio, HIGH);
	usleep(JUST_A_LITTLE_BIT);
	peripheral_gpio_write (gpio, LOW);
	peripheral_gpio_close (gpio);
}



