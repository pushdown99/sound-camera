/*********************************************************************************************

Special purpose Sound/Noise Detection Camera

Description:
	A SOURCE package to detect a noise around me and capture picture with camera.
	we will find out which places and points make a noise.
	because camera give a picture mapping noise heat map.

Files:
	adc-mcp3008.c	- mcp3008(ADC) control
	fft.c			- fast fourier(cosine/sine) transform
	soundcamera.c	- main module (base UI, logic)

References:
	* Tizen school (https://tizenschool.org/)

Copyright:
	Copyright(C) 2020 WB9LAB (wannabe 9, lab)
	email: haeyun@gmail.com
	download: https://github.com/pushdown99/sound-camera
	You may use, copy, modify this code for any purpose and without fee.

History:
	...
	Aug, 2020 : Edit the Sound/Noise Camera
	Sep, 2020 : Using FFT(Fast Fourier Transform)

 *******************************************************************************************/


#include <mcp3008.h>
#include "soundcam.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////

Eina_Bool doIdler(void *data) {
	appdata_t *ad = (appdata_t*)data;

	/* draw mini map */
	for (int i = 0; i < R_SENSOR; i++) {
		for (int j = 0; j < C_SENSOR; j++) {
			int k = (i * R_SENSOR) + j;
			if (ad->max4466[k].o) evas_object_color_set(ad->minimap[i][j], 255, 0, 0, ad->max4466[k].o);
			else                  evas_object_color_set(ad->minimap[i][j], 255, 0, 0, 5);
		}
	}

	/* draw heat map */
	for (int i = 0; i < R_HEATMAP; i++) {
		for (int j = 0; j < C_HEATMAP; j++) {
			evas_object_color_set(ad->heatmap[i][j], 255, 0, 0, ad->h[i][j]);
		}
	}
	if (ad->snapshot == true) {
		capCAM(data);
		ad->snapshot = false;
	}

	/* check ad->nfail */
	if(ad->nfail > THRESHOLD) {
		evas_object_hide (ad->conn);
		evas_object_show (ad->disc);
		ad->nfail   = 0;
		ad->network = 0;
	}
	if(ad->snap) {
		ad->snap = 0;
		capCAM(data);
	}

	return ECORE_CALLBACK_RENEW;
}

/*
 * Application event handler
 * =========================
 *
 * - create (initialize resource)
 * - control (call ecore_idler, not thread. because we need a exchanging appdata_t pointer)
 * - terminate (resource free)
 *
 */
bool initAPP (void *data)
{
	appdata_t *ad = (appdata_t*)data;
	ad->verbose = 0;
	ad->nfail 	= 0;
	ad->network = 1;
	ad->peak 	= 500;
	ad->freq 	= 1;
	ad->snap	= 0;

	initCURL   (data);
	initGUI    (data);
	initCAM    (data);
	initThread (data);

	return true;
}

void ctrlAPP(app_control_h app_control, void *data)
{
	appdata_t *ad = (appdata_t*)data;

	if (ad->idler) {
		ecore_idler_del(ad->idler);
		ad->idler = NULL;
	}
	ad->idler = ecore_idler_add(doIdler, data);
	if (!ad->idler) _E("Failed to add idler");
}

void termAPP (void *data)
{
	//ecore_evas_shutdown();
}


int main(int argc, char *argv[])
{
	_E("\n\n\n\n");
	_E("==============================================");
	_E("SOUNDCAMERA, WNB-TEAM                         ");
	_E("- started : %s", getTime ()                    );
	_E("- timstamp: %d", getUnixtime ()                );
	_E("==============================================");

	appdata_t ad;
	ui_app_lifecycle_callback_s event_callback = {0,};

	ad.idler = NULL;

	event_callback.create      = initAPP;
	event_callback.terminate   = termAPP;
	event_callback.app_control = ctrlAPP;
	return ui_app_main(argc, argv, &event_callback, &ad);
}
