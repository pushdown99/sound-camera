#include "soundcam.h"

/* MAX4466: microphone amplifier
 * we use this chip for proving sound and noise.
 *
 * notice.
 * output pin emit a analog voltages.
 * so we use a ADC (analog to digital converter) chip like MCP3008
 */

void mic_max4466_init (void)
{
	if (!adc_mcp3008_initialized) {
		adc_mcp3008_init();
		adc_mcp3008_initialized = 1;
	}
}

unsigned int mic_max4466_read (int channel)
{
	unsigned int v = 0;
	adc_mcp3008_read(channel, &v);
	return v;
}


