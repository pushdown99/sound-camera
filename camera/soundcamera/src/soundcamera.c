#include <camera.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <tizen.h>
#include <Ecore.h>
#include <curl/curl.h>
#include <peripheral_io.h>


#include "adc-mcp3008.h"
#include "soundcamera.h"

///////////////////////////////////////////////////////////////////////////////////////
//
// Date: 2020-09-05
// Author: WNB Team. (written by Haeyeon, Hwang)
// Description: We called SoundCamera. Because it detects a noise at our environment.
// License: All Copyleft.
//
///////////////////////////////////////////////////////////////////////////////////////

#define IMAGE_WIDTH 	640
#define IMAGE_HEIGHT 	480

#define IMAGE_QUALITY 	100

#define SENSOR_COL      4
#define SENSOR_ROW      3
#define SENSOR_MAX		(SENSOR_ROW*SENSOR_COL)

#define KERNEL          15 // needed odd value (like 1,3,5,...) 5 <- normal
#define CELL_ROW        (SENSOR_ROW*KERNEL)
#define CELL_COL        (SENSOR_COL*KERNEL)
#define CELL_MAX        (CELL_ROW*CELL_COL)
#define CELL_WIDTH      (IMAGE_WIDTH/CELL_COL)
#define CELL_HEIGHT     (IMAGE_HEIGHT/CELL_ROW)

#define FAILURE  		0
#define SUCCESS  		1

#define BUTTON_WIDTH    120
#define BUTTON_HEIGHT	BUTTON_WIDTH*SENSOR_ROW/SENSOR_COL
#define BUTTON_X        (IMAGE_WIDTH + 25)
#define BUTTON_Y        (IMAGE_HEIGHT/2 - BUTTON_HEIGHT/2)

#define MAP_WIDTH      	(BUTTON_WIDTH/SENSOR_COL)
#define MAP_HEIGHT     	(BUTTON_HEIGHT/SENSOR_ROW)
#define MAP_X			BUTTON_X
#define MAP_Y			40

#define LABEL_WIDTH		BUTTON_WIDTH
#define LABEL_HEIGHT    20
#define LABEL_X			BUTTON_X
#define LABEL_Y			5

#define EXIT_WIDTH    	120
#define EXIT_HEIGHT		50
#define EXIT_X        	(IMAGE_WIDTH + 25)
#define EXIT_Y        	IMAGE_HEIGHT - EXIT_HEIGHT - (3*LABEL_Y)

#define DURATION        1000
#define TIMEOUT			2 //(60*5) // Ecore timer timeout for Thinkspark

typedef struct {
	camera_h	ch;
} camera_t;

camera_t *cam = NULL;

typedef struct appdata {
   int  sensor[SENSOR_ROW][SENSOR_COL];

   Evas_Object *win;
   Evas_Object *evas;
   Evas_Object *view;
   Evas_Object *cell[CELL_ROW][CELL_COL];
   int  opaque[CELL_ROW][CELL_COL];

   Evas_Object *label;
   Evas_Object *map[SENSOR_ROW][SENSOR_COL];
   Evas_Object *btn;
   Evas_Object *ext;

   Ecore_Idler *idler;
   Ecore_Timer *timer;
   long ts;
} appdata_t;

CURL* 		curl;
CURLcode 	code;

#define BAR_VALUE_MAX 1023

static const char log_bar[129][128] =
{
	"|",
	"||",
	"|||",
	"||||",
	"|||||",
	"||||||",
	"|||||||",
	"||||||||",
	"|||||||||",
	"||||||||||",
	"|||||||||||",
	"||||||||||||",
	"|||||||||||||",
	"||||||||||||||",
	"|||||||||||||||",
	"||||||||||||||||",
	"|||||||||||||||||",
	"||||||||||||||||||",
	"|||||||||||||||||||",
	"||||||||||||||||||||",
	"|||||||||||||||||||||",
	"||||||||||||||||||||||",
	"|||||||||||||||||||||||",
	"||||||||||||||||||||||||",
	"|||||||||||||||||||||||||",
	"||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||",
	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
};

static void
print_bar(int index, unsigned int val)
{
	if (val > BAR_VALUE_MAX)
		val = BAR_VALUE_MAX;

	_E("[%03d] %4u %s", index, val, log_bar[val/8]);
}

////////////////////////////////////////////////////////////////////////////////////
//
// Custom Callback Function
// - cbResolution
// - cbPreview
// - cbCapture
// - cbCompleted
// - cbBtnDown/Up
//
static void
cbResolution (int width, int height, void *data)
{
	_E("Supported resolution: %d x %d (w, h)", width, height);
}

static void
cbPreview (camera_preview_data_s *frame, void *data)
{
    //_E("#planes: %d", frame->num_of_planes);
}

static void
cbCapture (camera_image_data_s* image, camera_image_data_s* postview, camera_image_data_s* thumbnail, void *data)
{
    _D("[capture] writing image to file");

    FILE *file = fopen("/tmp/x.jpg", "w+");
    if (image->data != NULL)
        fwrite(image->data, 1, image->size, file);
    fclose(file);
}

static void
cbCompleted (void *data)
{
	_E("[capture] completed.");
    usleep(25000); /* Display the captured image for 0.025 seconds */
    camera_start_preview(cam->ch);

}

static Eina_Bool
cbTimeout (void *data)
{
	appdata_t *ad = (appdata_t*)data;
	char url[256];
	int sum = 0;

	_E("cbTimeout call. %p", ad);

	for(int i = 0; i < SENSOR_ROW; i++)
		for (int j = 0; j < SENSOR_COL; j++) {
			sum += ad->sensor[i][j];
		}
	_E("cbTimeout call. sum=%d", sum);

	sprintf(url, "https://api.thingspark.kr/channels/entrys?apiKey=JmEeoADpTuDg2FZB&field1=%d", sum);

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	code = curl_easy_perform(curl);
    if(code != CURLE_OK)
    	_E ("curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
    curl_easy_cleanup(curl);

    return ECORE_CALLBACK_RENEW;
}

static void
postFile (void *data)
{
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	struct curl_slist *headerlist = NULL;
	static const char buf[] = "Expect:";

	curl_formadd(&formpost,
	               &lastptr,
	               CURLFORM_COPYNAME, "sampleFile",
	               CURLFORM_FILE, "/tmp/y.jpg",
	               CURLFORM_END);

	//curl_formadd(&formpost,
	//               &lastptr,
	//               CURLFORM_COPYNAME, "file",
	//               CURLFORM_COPYCONTENTS, "sampleFile",
	//               CURLFORM_END);


	curl_formadd(&formpost,
	               &lastptr,
	               CURLFORM_COPYNAME, "submit",
	               CURLFORM_COPYCONTENTS, "send",
	               CURLFORM_END);

	  curl = curl_easy_init();
	  headerlist = curl_slist_append(headerlist, buf);
	  if(curl) {
	    curl_easy_setopt(curl, CURLOPT_URL, "debian.tric.kr:9900/upload");
	    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
	    code = curl_easy_perform(curl);
	    if(code != CURLE_OK)
	    _E ("curl_easy_perform() failed: %s", curl_easy_strerror(code));
	    curl_easy_cleanup(curl);
	    curl_formfree(formpost);
	    curl_slist_free_all(headerlist);
	  }
}

static void
camCapture(appdata_t *ad)
{
    _E("[capture] call. %p", ad);
	camera_start_capture(cam->ch, cbCapture, cbCompleted, NULL);
	evas_object_image_save (ad->view, "/tmp/y.jpg", NULL, NULL); // preview w/ heatmap
	postFile (ad);
}

static void
cbBtnDown(void *data, Evas_Object *obj, void *event_info)
{
	appdata_t *ad = (appdata_t*)data;
	int ret;

	_E("#EVAS_CALLBACK_MOUSE_DOWN::BUTTON");
	evas_object_color_set(ad->btn, 68, 84, 76, 255);
	camCapture (ad);
}

static void
cbBtnUp(void *data, Evas_Object *obj, void *event_info)
{
	appdata_t *ad = (appdata_t*)data;
	_E("#EVAS_CALLBACK_MOUSE_UP::BUTTON");
	evas_object_color_set(ad->btn, 156, 124, 93, 255);
}

static void
cbExtDown(void *data, Evas_Object *obj, void *event_info)
{
	appdata_t *ad = (appdata_t*)data;
	_E("#EVAS_CALLBACK_MOUSE_DOWN::EXIT");
	for(int i=0; i< CELL_MAX; i++) ecore_evas_free(ad->cell[i]);
	ecore_evas_shutdown();
}


static void
readBtn (appdata_t *ad){
	peripheral_gpio_h gpio = NULL;
	int pin = 21;
	uint32_t value;

    peripheral_gpio_open(pin, &gpio);
	peripheral_gpio_set_direction(gpio, PERIPHERAL_GPIO_DIRECTION_OUT_INITIALLY_LOW);

	peripheral_gpio_read(gpio, &value);
	peripheral_gpio_close(gpio);

	_E("####readBtn: %d", value);
	if(value==1) {
		camCapture (ad);
	}
}

static int
initCamera (appdata_t *ad) {
	int ret = CAMERA_ERROR_NONE;
	camera_state_e state;

	if((cam = calloc(1, sizeof(camera_t))) == NULL) {
		_E("Fail to allocate memory");
		return FAILURE;
	}
	if ((ret = camera_create(CAMERA_DEVICE_CAMERA0, &(cam->ch))) != CAMERA_ERROR_NONE) {
		_E("Failed to create camera: ERR:%d", ret);
		return FAILURE;
	}
	if ((ret = camera_foreach_supported_preview_resolution(cam->ch, cbResolution, NULL)) != CAMERA_ERROR_NONE) {
		_E("Failed to get supported preview resolution: ERR:%d", ret);
		return FAILURE;
	}
	if ((ret = camera_set_preview_cb(cam->ch, cbPreview, ad)) != CAMERA_ERROR_NONE) {
		_E("Failed to set preview callback: E%d", ret);
		return FAILURE;
	}
	if ((ret = camera_set_preview_resolution(cam->ch, IMAGE_WIDTH, IMAGE_HEIGHT)) != CAMERA_ERROR_NONE) {
		_E("Failed to set preview resolution: E%d", ret);
		return FAILURE;
	}
	if ((ret = camera_attr_set_image_quality(cam->ch, IMAGE_QUALITY)) != CAMERA_ERROR_NONE) {
		_E("Failed to set image quality: E%d", ret);
		return FAILURE;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// NOTICE: Please check your camera state whether {CAMERA_STATE_CREATED, CAMERA_STATE_PREVIEW} or not
	//         Before "camera_set_display" function call.
	//
	if ((ret = camera_set_display(cam->ch, CAMERA_DISPLAY_TYPE_EVAS, GET_DISPLAY(ad->view))) != CAMERA_ERROR_NONE) {
		_E("camera_set_display failed: E%d", ret);
	    return FAILURE;
	}
	if (state != CAMERA_STATE_PREVIEW) {
		if ((ret = camera_start_preview(cam->ch)) != CAMERA_ERROR_NONE) {
			_E("Failed to start preview: E:%d", ret);
			return FAILURE;
		}
	}
	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Ordinary TIZEN Base UI routine.
// It called when you terminate your own application. (or implicitly termination)
//
static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_t *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// Create BaseUI Frame w/ EFL widget element
// - activate OpenGL
// - get Window basic Frame (give a rotate feature, portrait and landscape)
// - draw Camera view area (show, not hidden)
// - draw heatmap cell (show, not hidden. and
/////////////////////////////////////////////////////////////////////////////////////////////

static appdata_t*
create_base_gui(appdata_t *ad)
{
	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// Create EFL widget Application (w/ Base GUI)
	// - reference: https://docs.tizen.org/application/native/guides/multimedia/camera/
	//
	elm_config_accel_preference_set("opengl");
	ad->win = elm_win_add(NULL, PACKAGE, ELM_WIN_BASIC);

	elm_win_title_set(ad->win, "SoundCamera");
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}
	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	ad->evas = evas_object_evas_get(ad->win);
	evas_object_show(ad->win); // activate windows

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// Create EVAS Object for Camera Preview
	//
	ad->view = evas_object_image_add(ad->evas);
	evas_object_image_size_set(ad->view, IMAGE_WIDTH, IMAGE_HEIGHT);
	evas_object_image_fill_set(ad->view, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
	evas_object_resize(ad->view, IMAGE_WIDTH, IMAGE_HEIGHT);
	evas_object_show(ad->view);

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// Create EVAS Object for Audio sensor
	//
	for (int i=0; i<CELL_ROW; i++) {
		for(int j=0; j<CELL_COL; j++) {
			int x = (j%CELL_COL)*CELL_WIDTH ;
			int y = (i%CELL_ROW)*CELL_HEIGHT;
			//_D("draw object index(%d,%d) (X:%d, Y:%d)", i,j, x, y);
			Evas_Object *cell = evas_object_rectangle_add(ad->view);
			evas_object_resize(cell, CELL_WIDTH, CELL_HEIGHT);
			evas_object_move(cell, x, y);
			evas_object_color_set(cell, 0, 0, 0, 0);
			evas_object_render_op_set(cell, EVAS_RENDER_BLEND);
			evas_object_size_hint_weight_set(cell, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_show(cell);
			ad->cell[i][j]   = (Evas_Object *)cell;
			ad->opaque[i][j] = 0;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// Create EVAS Object for Noise map label
	//

	Evas_Object *label = evas_object_text_add(ad->evas);
	evas_object_text_font_set(label, "DejaVu", 18);
	evas_object_text_text_set(label, "noise map");
	evas_object_move(label, LABEL_X, LABEL_Y);
	evas_object_resize(label, LABEL_WIDTH, LABEL_HEIGHT);
	evas_object_color_set(label, 209, 209, 209, 255);
	//evas_object_render_op_set(label, EVAS_RENDER_COPY);
	//evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	//elm_object_content_set(ad->conform, ad->label);
	evas_object_show(label);
	ad->label = label;

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// Create EVAS Object for Noise map
	//
	for (int i=0; i<SENSOR_ROW; i++) {
		for(int j=0; j<SENSOR_COL; j++) {
			int x = MAP_X+(j%SENSOR_COL)*MAP_WIDTH ;
			int y = MAP_Y+(i%SENSOR_ROW)*MAP_HEIGHT;
			Evas_Object *map = evas_object_rectangle_add(ad->evas);
			evas_object_resize(map, MAP_WIDTH, MAP_HEIGHT);
			evas_object_move(map, x, y);
			evas_object_color_set(map, 255, 255, 255, 255);
			evas_object_render_op_set(map, EVAS_RENDER_COPY);
			evas_object_size_hint_weight_set(map, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_show(map);
			ad->map[i][j]   = (Evas_Object *)map;
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// Create EVAS Object for Shutter Button
	//
	Evas_Object *button = evas_object_rectangle_add(ad->evas);
	evas_object_resize(button, BUTTON_WIDTH, BUTTON_HEIGHT);
	evas_object_move(button, BUTTON_X, BUTTON_Y);
	evas_object_color_set(button, 156, 124, 93, 255);
	evas_object_render_op_set(button, EVAS_RENDER_BLEND);
	evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(button);
	ad->btn   = (Evas_Object *)button;

	evas_object_event_callback_add(ad->btn, EVAS_CALLBACK_MOUSE_DOWN, cbBtnDown, ad);
	evas_object_event_callback_add(ad->btn, EVAS_CALLBACK_MOUSE_UP,   cbBtnUp,   ad);

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// Create EVAS Object for Exit Button
	//
	Evas_Object *exit = evas_object_rectangle_add(ad->evas);
	evas_object_resize(exit, EXIT_WIDTH, EXIT_HEIGHT);
	evas_object_move(exit, BUTTON_X, EXIT_Y);
	evas_object_color_set(exit, 156, 124, 93, 255);
	evas_object_render_op_set(exit, EVAS_RENDER_BLEND);
	evas_object_size_hint_weight_set(exit, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(exit);
	ad->ext   = (Evas_Object *)exit;

	evas_object_event_callback_add(ad->ext, EVAS_CALLBACK_MOUSE_DOWN, cbExtDown, ad);

	return ad;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Probe sound and noise value w/ microphone sensor (MAX4466)
//
///////////////////////////////////////////////////////////////////////////////////////////////

//#define LOOPBACK

int cnt = 0;
static int initialized = 0;

#define SOUND_SAMPLING    	60
#define GUESS_NOISE_PITCH 	32
#define GUESS_NOISE_CNT 	(SOUND_SAMPLING/4)
#define GUESS_NOISE_AMP 	50

static int
getSoundValue (appdata_t *ad) {
	for(int i=0;i<SENSOR_ROW;i++)
		for(int j=0;j<SENSOR_COL;j++)
			ad->sensor[i][j] = 0;

#if defined(LOOPBACK)
	// get random sample sound value
	//ad->sensor[random()%SENSOR_ROW][random()%SENSOR_COL] = random()%1024;
    //ad->sensor[(cnt%SENSOR_MAX)%SENSOR_ROW][(cnt%SENSOR_MAX)%SENSOR_COL] = 128;
    ad->sensor[0][0] = 950;
    ad->sensor[1][0] = 870;
    ad->sensor[0][1] = 613;
    ad->sensor[1][1] = 1023;
    //ad->sensor[0][3] = 550;
	//cnt++;

	///////////////////////////////////////////////////////////////////////////////////////////
	//
	// normalize SPI value (0-1023) to (0-255)
	for(int i=0;i<SENSOR_ROW;i++)
		for(int j=0;j<SENSOR_COL;j++)
			ad->sensor[i][j] = ad->sensor[i][j]/4;
#else
	unsigned int read_value = 0;
	int ret = 0;

	if (!initialized) {
		ret = adc_mcp3008_init();
		retv_if(ret != 0, FAILURE);
		initialized = 1;
	}


	int sensor[SENSOR_ROW][SENSOR_COL] = {0, };   	// adjusted value
	int v[SENSOR_ROW][SENSOR_COL] = {0, };   		// adjusted value
	int p[SENSOR_ROW][SENSOR_COL] = {0, }; 			// previous
	int n[SENSOR_ROW][SENSOR_COL] = {0, }; 			// noise feature
	int a[SENSOR_ROW][SENSOR_COL] = {0, }; 			// amplifier


	for (int loop = 0; loop < SOUND_SAMPLING; loop++) {
		for (int i=0; i < SENSOR_ROW-1; i++)
			for(int j=0; j < SENSOR_COL; j++) {
				adc_mcp3008_read(i*SENSOR_COL+j, &read_value);
				if(i==0 && j==0) print_bar (loop, read_value);
                if(loop == 0) p[i][j] = read_value;

                if((abs(p[i][j] - read_value)) > GUESS_NOISE_PITCH) n[i][j] += 1;
                p[i][j] == read_value;
				a[i][j] = MAX(a[i][j], abs(read_value - 512));
			}
	}
	for (int i=0; i < SENSOR_ROW-1; i++)
		for(int j=0; j < SENSOR_COL; j++) {
			ad->sensor[i][j] = ((n[i][j] >= GUESS_NOISE_CNT) &&(a[i][j]>=GUESS_NOISE_AMP))? a[i][j]:0;
			_E("getSoundValue CH#%d [%d][%d] = %d (n:%d, a:%d)", i*SENSOR_COL+j,i,j,ad->sensor[i][j], n[i][j], a[i][j]);
		}
#endif
}

int flag =0;

static int
getOpaque(int mat[CELL_ROW][CELL_COL], int row, int col, int kernel) {
	int gap = (int)(kernel/2);
	int cnt=0, sum=0;
	for (int i=row-gap;i<=row+gap;i++) {
		for (int j=col-gap;j<=col+gap;j++) {
			cnt++;
			if(i<0) continue;
			if(j<0) continue;
			if(i>=CELL_ROW) continue;
			if(j>=CELL_COL) continue;
			sum += mat[i][j];
		}
	}
	//_E("getOpaque %d (sum:%d, cnt:%d)", sum/cnt, sum, cnt);
	return (sum/cnt);
}

static void
doBuildHeatMap (appdata_t *ad) {
	int mat[CELL_ROW][CELL_COL] = {0, };

	// spread sensor value to matrix
	for(int i=0;i<SENSOR_ROW;i++) {
		for(int j=0;j<SENSOR_COL;j++) {
			for(int k=0; k<KERNEL; k++) {
				for(int l=0;l<KERNEL; l++) {
					mat[(i*KERNEL)+k][(j*KERNEL)+l] = ad->sensor[i][j];
				}
			}
			flag++;
		}
	}
	for(int i=0;i<CELL_ROW;i++)
		for(int j=0;j<CELL_COL;j++)
			ad->opaque[i][j] = getOpaque(mat, i, j, KERNEL*3/2);


	//for(int i=0;i<CELL_ROW;i++)
	//	for(int j=0;j<CELL_COL;j++)
	//		ad->opaque[i][j] = mat[i][j];

}

static long
getTimestamp (void) {
	struct timespec time_s;

	clock_gettime(CLOCK_MONOTONIC, &time_s);
	return (time_s.tv_sec*1000 + time_s.tv_nsec/1000000);
}

static void
doReadIt(appdata_t *ad) {
	long now = getTimestamp ();

	if(now < (ad->ts + DURATION)) return;
	ad->ts = now;

	readBtn (ad);
	getSoundValue(ad);
	doBuildHeatMap(ad);

	for(int i=0;i<CELL_ROW;i++)
		for(int j=0;j<CELL_COL;j++)
			evas_object_color_set(ad->cell[i][j], 255, 0, 0, ad->opaque[i][j]);

	for(int i=0;i<SENSOR_ROW;i++)
		for(int j=0;j<SENSOR_COL;j++) {
			//_E("map [%d,%d] = %d", i, j, ad->sensor[i][j]);
			if(ad->sensor[i][j] < 50)
				evas_object_color_set(ad->map[i][j], 209, 209, 209, 255);
			else
				evas_object_color_set(ad->map[i][j], 255, 0, 0, ad->sensor[i][j]);
		}
}


////////////////////////////////////////////////////////////////////////////////////////////////
//
// Application Event Handler
// - create (initialize resource)
// - control (call encore-idler, not thread. because we need a exchanging appdata_t pointer)
// - terminate (resource free)

static bool
app_create(void *data)
{
	appdata_t *ad = (appdata_t*)data;
	ad->ts = getTimestamp ();

	create_base_gui (ad);
	initCamera (ad);
	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	appdata_t *ad = (appdata_t*)data;

	if (ad->idler) {
		ecore_idler_del(ad->idler);
		ad->idler = NULL;
	}

	ad->idler = ecore_idler_add(doReadIt, ad);
	if (!ad->idler)
		_E("Failed to add idler");

	ad->timer = ecore_timer_add (TIMEOUT, cbTimeout, ad);
    return;
}

static void
app_terminate(void *data)
{
	appdata_t *ad = (appdata_t*)data;
	for(int i=0; i< CELL_MAX; i++) ecore_evas_free(ad->cell[i]);
	ecore_evas_shutdown();
}

static char* getTime() { time_t t = time(0); return ctime(&t); }

int
main(int argc, char *argv[])
{
	_E("\n\n\n\n");
	_E("==============================================");
	_E("SOUNDCAMERA, WNB-TEAM                         ");
	_E("- started: %s", getTime());
	_E("==============================================");

	curl_global_init(CURL_GLOBAL_ALL);
	appdata_t ad;
	ui_app_lifecycle_callback_s event_callback = {0,};

	ad.idler = NULL;

	event_callback.create      = app_create;
	event_callback.terminate   = app_terminate;
	event_callback.app_control = app_control;
	return ui_app_main(argc, argv, &event_callback, &ad);
}
