#include "soundcam.h"

/* baseUI callback function */
static void
cbShotDown(void *data, struct _Eo_Opaque *obj1, struct _Eo_Opaque *obj2, void *info)
{
	appdata_t *ad = (appdata_t *)data;
	evas_object_hide (ad->shot);
	evas_object_show (ad->shot2);
	capCAM (data);
}

static void
cbShotUp(void *data, struct _Eo_Opaque *obj1, struct _Eo_Opaque *obj2, void *info)
{
	appdata_t *ad = (appdata_t *)data;
	evas_object_hide (ad->shot2);
	evas_object_show (ad->shot);
}

static void
cbConnDown(void *data, struct _Eo_Opaque *obj1, struct _Eo_Opaque *obj2, void *info)
{
	appdata_t *ad = (appdata_t *)data;
	ad->network = 0;
	evas_object_hide (ad->conn);
	evas_object_show (ad->disc);
}

static void
cbDiscDown(void *data, struct _Eo_Opaque *obj1, struct _Eo_Opaque *obj2, void *info)
{
	appdata_t *ad = (appdata_t *)data;
	ad->network = 1;
	evas_object_hide (ad->disc);
	evas_object_show (ad->conn);
}

static void
cbDebugDown(void *data, struct _Eo_Opaque *obj1, struct _Eo_Opaque *obj2, void *info)
{
	appdata_t *ad = (appdata_t *)data;
	ad->verbose = (ad->verbose)? 0 : 1;
	_E("verbose mode = %s", (ad->verbose)? "On": "Off");
	if(ad->verbose) {
		evas_object_hide (ad->debug2);
		evas_object_show (ad->debug);
	}
	else {
		evas_object_hide (ad->debug);
		evas_object_show (ad->debug2);
	}
}

static void
cbExitDown(void *data, struct _Eo_Opaque *obj1, struct _Eo_Opaque *obj2, void *info)
{
	appdata_t *ad = (appdata_t *)data;
	//ecore_evas_shutdown();
	elm_win_lower(ad->win);
}


/* Ordinary TIZEN Base UI routine.
 * It called when you terminate your own application. (or implicitly termination)
 */
static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_t *ad = (appdata_t *)data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static Eina_Bool
cbThumbnail (void *data)
{
	appdata_t *ad = (appdata_t*)data;
    char pathCapture[128];
	sprintf(pathCapture, "/tmp/%s", CAPTURE_JPG);

    evas_object_image_file_set(ad->thumbnail, pathCapture, NULL);
    evas_object_image_fill_set(ad->thumbnail, 0, 0, M_WIDTH, M_HEIGHT);
    evas_object_resize(ad->thumbnail, M_WIDTH, M_HEIGHT);
    evas_object_move (ad->thumbnail, WIDTH + BORDER, M_HEIGHT + BORDER);
	evas_object_show (ad->thumbnail);

    return ECORE_CALLBACK_RENEW;
}

/*
 * Create BaseUI Frame w/ EFL widget element
 * =========================================
 *
 * - activate OpenGL
 * - get Window basic Frame (give a rotate feature, portrait and landscape)
 * - draw Camera view area
 * - draw heatmap cell
 *
 */
void initGUI (void *data)
{
	appdata_t *ad = (appdata_t *)data;
    char pathShot[128], pathShot2[128];
    char pathExit[128], pathCapture[128];
    char pathConn[128], pathDisc[128];
    char pathDebug[128], pathDebug2[128];

	sprintf(pathShot, "%s/%s", 	app_get_shared_resource_path(), SHOT_PNG);
	sprintf(pathShot2, "%s/%s", app_get_shared_resource_path(), SHOT2_PNG);
	sprintf(pathExit, "%s/%s", 	app_get_shared_resource_path(), EXIT_PNG);
	sprintf(pathConn, "%s/%s", 	app_get_shared_resource_path(), CONN_PNG);
	sprintf(pathDisc, "%s/%s", 	app_get_shared_resource_path(), DISC_PNG);
	sprintf(pathDebug, "%s/%s", app_get_shared_resource_path(), DEBUG_PNG);
	sprintf(pathDebug2,"%s/%s", app_get_shared_resource_path(), DEBUG2_PNG);
	sprintf(pathCapture, "/tmp/%s", CAPTURE_JPG);

	/*
	 * create EFL widget application (w/ base GUI)
	 * - reference: https://docs.tizen.org/application/native/guides/multimedia/camera/
	 */
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

	///////////////////////////////////////////////////////////////////////////////////////////////

	ad->evas = evas_object_evas_get(ad->win);
	evas_object_show(ad->win); // activate windows

	/* Create EVAS Object for Camera Preview */
	ad->view = evas_object_image_add(ad->evas);
	evas_object_image_size_set (ad->view, WIDTH, HEIGHT);
	evas_object_image_fill_set (ad->view, 0, 0, WIDTH, HEIGHT);
	evas_object_resize (ad->view, WIDTH, HEIGHT);
	evas_object_show (ad->view);


	/* Create EVAS Object for Control area */
	Evas_Object *ctrl = evas_object_rectangle_add (ad->evas);
	evas_object_resize(ctrl, M_WIDTH + 2 * BORDER, HEIGHT);
	evas_object_move (ctrl, WIDTH, 0);
	evas_object_color_set (ctrl, 255, 255, 255, 255); // 220 -> 255
	evas_object_render_op_set (ctrl, EVAS_RENDER_COPY);
	evas_object_size_hint_weight_set (ctrl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show (ctrl);
	ad->ctrl = (Evas_Object *) ctrl;


	/* create EVAS object for heat map */
	for (int i = 0; i < R_HEATMAP; i++) {
		for(int j = 0; j < C_HEATMAP; j++) {
			int x = (j % C_HEATMAP) * H_WIDTH ;
			int y = (i % R_HEATMAP) * H_HEIGHT;
			Evas_Object *heatmap = evas_object_rectangle_add (ad->view);
			evas_object_resize (heatmap, H_WIDTH, H_HEIGHT);
			evas_object_move (heatmap, x, y);
			evas_object_color_set (heatmap, 0, 0, 0, 0);
			evas_object_render_op_set (heatmap, EVAS_RENDER_BLEND);
			evas_object_size_hint_weight_set (heatmap, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_show (heatmap);
			ad->heatmap[i][j]   = (Evas_Object *) heatmap;
		}
	}

	/* create EVAS object for noise map */
	for (int i=0; i<R_SENSOR; i++) {
		for(int j=0; j<C_SENSOR; j++) {
			int x = WIDTH + BORDER +  (j % C_SENSOR) * (M_WIDTH/C_SENSOR) ;
			int y = (i % R_SENSOR) * (M_HEIGHT/R_SENSOR);

			Evas_Object *overlay = evas_object_rectangle_add (ad->evas);
			evas_object_resize (overlay, M_WIDTH/C_SENSOR - 2, M_HEIGHT/R_SENSOR - 2);
			evas_object_move (overlay, x + 1, y + 1);
			evas_object_color_set (overlay, 0, 0, 0, 255);
			evas_object_render_op_set (overlay, EVAS_RENDER_COPY);
			evas_object_size_hint_weight_set (overlay, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_show (overlay);
			ad->overlay[i][j] = (Evas_Object *)overlay;

			Evas_Object *minimap = evas_object_rectangle_add (ad->evas);
			evas_object_resize (minimap, M_WIDTH/C_SENSOR - 4, M_HEIGHT/R_SENSOR - 4);
			evas_object_move (minimap, x + 2, y + 2);
			evas_object_color_set (minimap, random()%255, random()%255, random()%255, 255);
			evas_object_render_op_set (minimap, EVAS_RENDER_BLEND);
			evas_object_size_hint_weight_set (minimap, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_show (minimap);
			ad->minimap[i][j] = (Evas_Object *)minimap;

		}
	}

	/* create EVAS object for thumbnail */
	Evas_Object *thumbnail = evas_object_image_add (ad->evas);
	evas_object_image_size_set(thumbnail, M_WIDTH, M_HEIGHT);
    evas_object_image_file_set(thumbnail, pathCapture, NULL);
    evas_object_image_fill_set(thumbnail, 0, 0, M_WIDTH, M_HEIGHT);
    evas_object_resize(thumbnail, M_WIDTH, M_HEIGHT);
    evas_object_move (thumbnail, WIDTH + BORDER, M_HEIGHT + BORDER + 10);
	evas_object_show (thumbnail);
	ad->thumbnail = (Evas_Object *) thumbnail;

	/* create EVAS object for shot & shutter */
	Evas_Object *shot = evas_object_image_add (ad->evas);
    evas_object_image_file_set(shot, pathShot, NULL);
    evas_object_image_fill_set(shot, 0, 0, B_WIDTH, B_HEIGHT);
    evas_object_resize(shot, B_WIDTH, B_HEIGHT);
    evas_object_move (shot, WIDTH + BORDER, HEIGHT/2);
	evas_object_show (shot);
	ad->shot = (Evas_Object *) shot;

	evas_object_event_callback_add(ad->shot, EVAS_CALLBACK_MOUSE_DOWN, cbShotDown, (void*)ad);
	evas_object_event_callback_add(ad->shot, EVAS_CALLBACK_MOUSE_UP,   cbShotUp,   (void*)ad);

	/* create EVAS object for shot & shutter */
	Evas_Object *shot2 = evas_object_image_add (ad->evas);
    evas_object_image_file_set(shot2, pathShot2, NULL);
    evas_object_image_fill_set(shot2, 0, 0, B_WIDTH, B_HEIGHT);
    evas_object_resize(shot2, B_WIDTH, B_HEIGHT);
    evas_object_move (shot2, WIDTH + BORDER, HEIGHT/2);
	evas_object_show (shot2);
	evas_object_hide (shot2);
	ad->shot2 = (Evas_Object *) shot2;

	evas_object_event_callback_add(ad->shot2, EVAS_CALLBACK_MOUSE_DOWN, cbShotDown, (void*)ad);
	evas_object_event_callback_add(ad->shot2, EVAS_CALLBACK_MOUSE_UP,   cbShotUp,   (void*)ad);

	/* Create EVAS Object for Connect Button */
	Evas_Object *conn = evas_object_image_add (ad->evas);
    evas_object_image_file_set(conn, pathConn, NULL);
    evas_object_image_fill_set(conn, 0, 0, I_WIDTH, I_HEIGHT);
    evas_object_resize(conn, I_WIDTH, I_HEIGHT);
    evas_object_move (conn, WIDTH + BORDER, HEIGHT - I_HEIGHT - 10);
	evas_object_show (conn);
	ad->conn = (Evas_Object *) conn;

	evas_object_event_callback_add(ad->conn, EVAS_CALLBACK_MOUSE_DOWN, cbConnDown, ad);

	/* Create EVAS Object for Disconnect Button */
	Evas_Object *disc = evas_object_image_add (ad->evas);
    evas_object_image_file_set(disc, pathDisc, NULL);
    evas_object_image_fill_set(disc, 0, 0, I_WIDTH, I_HEIGHT);
    evas_object_resize(disc, I_WIDTH, I_HEIGHT);
    evas_object_move (disc, WIDTH + BORDER, HEIGHT - I_HEIGHT - 10);
	evas_object_hide (disc);
	ad->disc = (Evas_Object *) disc;

	evas_object_event_callback_add(ad->disc, EVAS_CALLBACK_MOUSE_DOWN, cbDiscDown, ad);

	/* Create EVAS Object for Debug Button */
	Evas_Object *debug = evas_object_image_add (ad->evas);
    evas_object_image_file_set(debug, pathDebug, NULL);
    evas_object_image_fill_set(debug, 0, 0, I_WIDTH, I_HEIGHT);
    evas_object_resize(debug, I_WIDTH, I_HEIGHT);
    evas_object_move (debug, WIDTH + BORDER + I_GAP + I_WIDTH, HEIGHT - I_HEIGHT - 10);
	evas_object_hide (debug);
	ad->debug = (Evas_Object *) debug;

	evas_object_event_callback_add(ad->debug, EVAS_CALLBACK_MOUSE_DOWN, cbDebugDown, ad);

	/* Create EVAS Object for Debug Button */
	Evas_Object *debug2 = evas_object_image_add (ad->evas);
    evas_object_image_file_set(debug2, pathDebug2, NULL);
    evas_object_image_fill_set(debug2, 0, 0, I_WIDTH, I_HEIGHT);
    evas_object_resize(debug2, I_WIDTH, I_HEIGHT);
    evas_object_move (debug2, WIDTH + BORDER + I_GAP + I_WIDTH, HEIGHT - I_HEIGHT - 10);
	evas_object_show (debug2);
	ad->debug2 = (Evas_Object *) debug2;

	evas_object_event_callback_add(ad->debug2, EVAS_CALLBACK_MOUSE_DOWN, cbDebugDown, ad);

	/* Create EVAS Object for Exit Button */
	Evas_Object *exit = evas_object_image_add (ad->evas);
    evas_object_image_file_set(exit, pathExit, NULL);
    evas_object_image_fill_set(exit, 0, 0, I_WIDTH, I_HEIGHT);
    evas_object_resize(exit, I_WIDTH, I_HEIGHT);
    evas_object_move (exit, WIDTH + BORDER + I_GAP + I_GAP + I_WIDTH + I_WIDTH, HEIGHT - I_HEIGHT - 10);
	evas_object_show (exit);
	ad->exit = (Evas_Object *) exit;


	evas_object_event_callback_add(ad->exit, EVAS_CALLBACK_MOUSE_DOWN, cbExitDown, ad);

	ad->timer = ecore_timer_add (TIMEOUT, cbThumbnail, ad);
	usleep(2000000);
}

void termGUI (void *data)
{
	appdata_t *ad = (appdata_t *)data;

	// remove callback event
	evas_object_event_callback_del(ad->shot, 	EVAS_CALLBACK_MOUSE_DOWN, 	cbShotDown	);
	evas_object_event_callback_del(ad->shot, 	EVAS_CALLBACK_MOUSE_UP, 	cbShotUp	);
	evas_object_event_callback_del(ad->conn, 	EVAS_CALLBACK_MOUSE_DOWN, 	cbConnDown	);
	evas_object_event_callback_del(ad->disc, 	EVAS_CALLBACK_MOUSE_DOWN, 	cbDiscDown	);
	evas_object_event_callback_del(ad->debug, 	EVAS_CALLBACK_MOUSE_DOWN, 	cbDebugDown	);
	evas_object_event_callback_del(ad->exit, 	EVAS_CALLBACK_MOUSE_DOWN, 	cbExitDown	);

	// remove image object
	evas_object_del(ad->shot);
	evas_object_del(ad->conn);
	evas_object_del(ad->disc);
	evas_object_del(ad->debug);
	evas_object_del(ad->exit);

	// remove noise map
	for (int i=0; i<R_SENSOR; i++) {
		for(int j=0; j<C_SENSOR; j++) {
			evas_object_del(ad->minimap[i][j]);
		}
	}

	// remove heat map
	for (int i = 0; i < R_HEATMAP; i++) {
		for(int j = 0; j < C_HEATMAP; j++) {
			evas_object_del(ad->heatmap[i][j]);
		}
	}


	// remove control
	evas_object_del(ad->ctrl);

	// remove camera preview
	evas_object_del(ad->view);
}


