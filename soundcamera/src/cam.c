#include "soundcam.h"

camera_t*	cam = NULL;

/*
 * Custom Callback Function
 * ========================
 *
 * - cbResolution
 * - cbPreview
 * - cbCapture
 * - cbCompleted
 * - cbBtnDown/Up
 */
static bool
cbResolution (int width, int height, void *data)
{
	return true;
}

static void
cbPreview (camera_preview_data_s *frame, void *data)
{
}

static void
cbCapture (camera_image_data_s* image, camera_image_data_s* postview, camera_image_data_s* thumbnail, void *data)
{
	char pathPreview[64];

	sprintf(pathPreview, "/tmp/%s", PREVIEW_JPG);

	FILE *file = fopen(pathPreview, "w+");
    if (image->data != NULL)
        fwrite(image->data, 1, image->size, file);
    fclose(file);
}

static void
cbCompleted (void *data)
{
    usleep (25000); /* Display the captured image for 0.025 seconds */
    camera_start_preview (cam->ch);

}

void capCAM (void *data)
{
	appdata_t *ad = (appdata_t*)data;
	char pathCapture[64], pathWindow[64];

	sprintf(pathCapture, "/tmp/%s", CAPTURE_JPG);
	sprintf(pathWindow,  "/tmp/%s", WINDOW_JPG );
	shutter();
	camera_start_capture	(cam->ch, cbCapture, cbCompleted, NULL);
	evas_object_image_save 	(ad->view, pathCapture, NULL, NULL); // Preview + Heatmap overlayed.
	evas_object_image_save 	(ad->win,  pathWindow,  NULL, NULL); // Preview + Heatmap overlayed.
	curlPostFile (data, IMAGE_UPLOAD_URL, pathCapture);
}

/* Initialize Camera device and setup preview feature */
int initCAM (void *data) {
	appdata_t *ad = (appdata_t*)data;
	camera_flip_e camera_default_flip = CAMERA_FLIP_BOTH;
	ad->snapshot = false;

	int ret = CAMERA_ERROR_NONE;

	if((cam = calloc(1, sizeof(camera_t))) == NULL) {
		if(ad->verbose) _E("Fail to allocate memory");
		return FAILURE;
	}
	if ((ret = camera_create(CAMERA_DEVICE_CAMERA0, &(cam->ch))) != CAMERA_ERROR_NONE) {
		if(ad->verbose) _E("Failed to create camera: ERR:%d", ret);
		return FAILURE;
	}
	if ((ret = camera_set_display_flip(cam->ch, camera_default_flip)) != CAMERA_ERROR_NONE) {
		if(ad->verbose) _E("Failed to set display flip: ERR:%d", ret);
		return FAILURE;
	}
	if ((ret = camera_foreach_supported_preview_resolution(cam->ch, cbResolution, NULL)) != CAMERA_ERROR_NONE) {
		if(ad->verbose) _E("Failed to get supported preview resolution: ERR:%d", ret);
		return FAILURE;
	}
	if ((ret = camera_set_preview_cb(cam->ch, cbPreview, ad)) != CAMERA_ERROR_NONE) {
		if(ad->verbose) _E("Failed to set preview callback: E%d", ret);
		return FAILURE;
	}
	if ((ret = camera_set_preview_resolution(cam->ch, WIDTH, HEIGHT)) != CAMERA_ERROR_NONE) {
		if(ad->verbose) _E("Failed to set preview resolution: E%d", ret);
		return FAILURE;
	}
	if ((ret = camera_attr_set_image_quality(cam->ch, QUALITY)) != CAMERA_ERROR_NONE) {
		if(ad->verbose) _E("Failed to set image quality: E%d", ret);
		return FAILURE;
	}

	if ((ret = camera_set_display(cam->ch, CAMERA_DISPLAY_TYPE_EVAS, GET_DISPLAY(ad->view))) != CAMERA_ERROR_NONE) {
		if(ad->verbose) _E("camera_set_display failed: E%d", ret);
	    return FAILURE;
	}
	if ((ret = camera_start_preview(cam->ch)) != CAMERA_ERROR_NONE) {
		if(ad->verbose) _E("Failed to start preview: E:%d", ret);
		return FAILURE;
	}
	return SUCCESS;
}

/* Terminate Camera  preview feature */
void termCAM (void *data)
{
	camera_stop_preview(cam->ch);
}
