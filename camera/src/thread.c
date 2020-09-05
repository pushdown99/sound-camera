#include "soundcam.h"

/********************************************************************************************
 *
 * Reference:
 * - https://docs.tizen.org/iot/api/5.0/tizen-iot-headed/ecore_thread_example_c.html
 *
 */

/********************************************************************************************
 *
 *	MAX4466 sensor handling (SPI) thread
 *	------------------------------------
 */
int noiseDetect(char* data, int peak, int frequency)
{
	if(peak >= 900) 	return 205;
	if(peak >= 800) 	return 185;
	if(peak >= 700) 	return 165;

	if(frequency >= 15) return 205;
	if(frequency >= 14) return 185;
	if(frequency >= 13) return 165;
	if(frequency >= 12) return 145;
	if(frequency >= 11) return 125;
	if(frequency >= 10) return 105;

	//if(peak >= 800 && frequency >= (NSAMPLE/4)) return 250;
	//if(peak >= 600 && frequency >= (NSAMPLE/8)) return 200;
	//if(peak >= 600 || frequency >= (NSAMPLE/8)) return 150;
	return 0;
}

/* Callback <main> function for MAX4466 (microphone) sensor */
void cbMThread (void *data EINA_UNUSED, Ecore_Thread *th)
{
	max4466_t *mp =(max4466_t*)data;
	appdata_t *ad = (appdata_t*)mp->data;
	char in[4096], out[4096];

	while(FOREVER) {
		unsigned int peak = 500;

		/* Read MAX4466 output value (not voltage) w/ MCP3008 ADC */
		for(int i = 0; i < NSAMPLE; i++) {
			mp->v[i] = (mic_max4466_read(mp->id)/20)*20;
			mp->a[i] = ((double)mp->v[i])/1024; 	// 0 <= sample value < 1
			mp->f[i] = 0;							// initialize
			peak = MAX(peak, mp->v[i]);
			if(ad->verbose && mp->id == 0) print_bar(mp->id, mp->v[i]);
		}

		/* Note peak strength */
		mp->p = peak;

		unsigned int freq = 0;

		/* Post sensor value to server at GCP(Google Cloud Platform) (HTTP/POST) */
		sprintf (out, "channel=%d&data=[%s]&peak=%d", mp->id, dumpUInt32((uint32_t*)mp->v, NSAMPLE, in), peak);
		curlPostData (mp->data, SENSOR_POST_URL, out);

		/* Using complex discrete fourier transform */
		cdft(NSAMPLE,  1, mp->a);
		cdft(NSAMPLE, -1, mp->a);

		/* fouriere transform matrix mapping to frequency graph for visualization. */
		for(int i = 0; i < NSAMPLE; i++) mp->f[(int)(mp->a[i]*2)] += 1;
		for(int i = 0; i < NSAMPLE; i++) if(mp->f[i] > 0) freq += 1;

		/* Note frequency */
		mp->n = freq;

		/* Post FFT value to server at GCP (HTTP/POST) */
		sprintf (out, "channel=%d&data=[%s]&freq=%d", mp->id, dumpUInt32((uint32_t*)mp->f, NSAMPLE, in), freq);
		curlPostData (mp->data, FFT_POST_URL, out);

		((appdata_t*)mp->data)->peak = MAX(((appdata_t*)mp->data)->peak, mp->p);
		((appdata_t*)mp->data)->freq = MAX(((appdata_t*)mp->data)->freq, mp->n);

		/* Calculation Noise */
		mp->o = noiseDetect (data, mp->p, mp->n);  // return noise strength (0-255)

		if (ecore_thread_check(th)) break;
		usleep(500000);
	}
}

/* Callback <end> function for MAX4466 (microphone) sensor */
void
cbMThreadEnd (void *data, Ecore_Thread *th)
{
}

/* Callback <cancel> function for MAX4466 (microphone) sensor */
void
cbMThreadCancel (void *data, Ecore_Thread *th)
{
}

/********************************************************************************************
 *
 *	Button handling (GPIO) thread
 *	-----------------------------
 */

/* Callback <main> function for Button */
void cbBThread (void *data EINA_UNUSED, Ecore_Thread *th)
{
	peripheral_gpio_h gpio = NULL;
	uint32_t value;
	appdata_t *ad = (appdata_t*)data;

    peripheral_gpio_open(GPIO_BUTTON, &gpio);
	peripheral_gpio_set_direction(gpio, PERIPHERAL_GPIO_DIRECTION_OUT_INITIALLY_LOW);

	while(FOREVER) {
		peripheral_gpio_read(gpio, &value);
		if(value == PUSH) {
			//capCAM (data);
			ad->snapshot = true;
			usleep(500000);
		}
		if (ecore_thread_check(th)) break;
		usleep(20);
	}
	peripheral_gpio_close(gpio);
}

/* Callback <end> function for Button */
void
cbBThreadEnd (void *data, Ecore_Thread *th)
{
}

/* Callback <cancel> function for Button */
void
cbBThreadCancel (void *data, Ecore_Thread *th)
{
}

/********************************************************************************************
 *
 *	Coordinator thread
 *	-----------------------------
 */

int getOpaque(int m[R_HEATMAP][C_HEATMAP], int row, int col, int kernel) {
	int gap = (int)(kernel / 2);
	int cnt = 0, sum = 0;
	for (int i = row - gap; i <= row + gap; i++) {
		for (int j = col - gap; j <= col + gap;j++) {
			if(i < 0) continue;
			if(j < 0) continue;
			if(i >= R_HEATMAP) continue;
			if(j >= C_HEATMAP) continue;
			cnt++;
			sum += m[i][j];
		}
	}
	return (sum/cnt);
}

/* Callback <main> function for Coordinator */
void cbCThread (void *data EINA_UNUSED, Ecore_Thread *th)
{
	appdata_t *ad = (appdata_t*)data;

	while (FOREVER) {
		int m[R_HEATMAP][C_HEATMAP];

		/* spread sensor value to m */
		for (int i = 0; i < R_HEATMAP; i++) {
			for (int j = 0; j < C_HEATMAP; j++) {
				int x = i / HEATMAP, y = j / HEATMAP;
				int k = (x * C_SENSOR) + y;
				m[i][j] = ad->max4466[k].o;
			}
		}
		/* get heat map opaque, interference by neighbor cell */
		for(int i = 0; i < R_HEATMAP; i++) {
			for(int j = 0; j < C_HEATMAP; j++) {
				ad->h[i][j] = getOpaque((int (*)[])m, i, j, KERNEL*3/2);
			}
		}

		if (ecore_thread_check(th)) break;
		usleep(1000000);
	}
}

/* Callback <end> function for Coordinator */
void
cbCThreadEnd (void *data, Ecore_Thread *th)
{
}

/* Callback <cancel> function for Coordinator */
void
cbCThreadCancel (void *data, Ecore_Thread *th)
{
}

/********************************************************************************************
 *
 *	Dashboard thread
 *	-----------------------------
 */

/* Callback <main> function for Dashboard (ThingsPark) */
void cbDThread (void *data EINA_UNUSED, Ecore_Thread *th)
{
	appdata_t *ad = (appdata_t*)data;

	while (FOREVER) {
		char url[512];
		sprintf(url, "%s?apiKey=%s&field1=%d&field2=%d", THINGSPARK_URL, THINGSPARK_KEY, ad->peak, ad->freq);
		curlPost(data, url);
		ad->peak = 500;
		ad->freq = 1;

		if (ecore_thread_check(th)) break;
		usleep(30 * 1000000);	// periodic report per 30 seconds
	}
}

/* Callback <end> function for Dashboard */
void
cbDThreadEnd (void *data, Ecore_Thread *th)
{
}

/* Callback <cancel> function for Dashboard */
void
cbDThreadCancel (void *data, Ecore_Thread *th)
{
}

/********************************************************************************************
 *
 *	Thread control functions
 *	------------------------
 */

void initThread (void *data)
{
	appdata_t *ad = (appdata_t*)data;
	if(ad->verbose) _E("max #threads = %d", ecore_thread_max_get());

	mic_max4466_init ();

	for (int id=0; id < NSENSOR; id++) {
		ad->max4466[id].id 		= id;
		ad->max4466[id].o 		= 0;
		ad->max4466[id].data 	= (void*)ad;
		ecore_thread_run(cbMThread, cbMThreadEnd, cbMThreadCancel, &ad->max4466[id]);
		usleep(100);
	}
	ecore_thread_run(cbBThread, cbBThreadEnd, cbBThreadCancel, data);  // button event monitor
	ecore_thread_run(cbCThread, cbCThreadEnd, cbCThreadCancel, data);  // coordinator for screen drawing
	ecore_thread_run(cbDThread, cbDThreadEnd, cbDThreadCancel, data);  // dashboard (thingspark)
}

void termThread (void *data)
{
}
