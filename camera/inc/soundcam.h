#ifndef __soundcamera_H__
#define __soundcamera_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <camera.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>
#include <app.h>
#include <app_common.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <tizen.h>
#include <Ecore.h>
#include <curl/curl.h>
#include <peripheral_io.h>

#include "cam.h"
#include "misc.h"
#include "post.h"
#include "piezoe.h"
#include "fourier.h"
#include "baseUI.h"
#include "thread.h"
#include "debug.h"
#include "max4466.h"
#include "mcp3008.h"

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "soundcamera"

#if !defined(PACKAGE)
#define PACKAGE "org.example.soundcamera"
#endif

#define _E(fmt, args...) dlog_print(DLOG_ERROR, LOG_TAG, "%s : %s(%d) > "fmt"\n", rindex(__FILE__, '/') + 1, __func__, __LINE__, ##args)
#define _D(fmt, args...) dlog_print(DLOG_DEBUG, LOG_TAG, "%s : %s(%d) > "fmt"\n", rindex(__FILE__, '/') + 1, __func__, __LINE__, ##args)
#define retv_if(expr, val) do { if (expr) { _E("(%s) -> %s() return", #expr, __FUNCTION__); return (val); } } while (0)

#define FAILURE  		0
#define SUCCESS  		1

#define FOREVER			1

#define NSAMPLE			128 				// SOUND SIGNAL SAMPLING RATE, 2^m
#define NSENSOR			8					// NSENSOR = C_SENSOR (COL) x R_SENSOR (ROW)
#define C_SENSOR		4
#define R_SENSOR		2
#define WIDTH			640					// PREVIEW IMAGE WIDTH
#define HEIGHT          480					// PREVIEW IMAGE HEIGHT
#define QUALITY 		100
#define S_WIDTH         (WIDTH/C_SENSOR)  	// SENSOR WIDTH  (SENSOR MAPPED), 160
#define S_HEIGHT        (HEIGHT/R_SENSOR)   // SENSOR HEIGHT (SENSOR MAPPED), 240
#define HEATMAP         10                  // MATRIX (10 x 10)
#define H_WIDTH         (S_WIDTH/HEATMAP)   // HEATMAP CELL WIDTH, 16
#define H_HEIGHT        (S_HEIGHT/HEATMAP)  // HEATMAP CELL HEIGHT, 24
#define C_HEATMAP		(C_SENSOR*HEATMAP)
#define R_HEATMAP		(R_SENSOR*HEATMAP)
#define M_WIDTH			140
#define M_HEIGHT        105

#define B_WIDTH			140 	// shutter button
#define B_HEIGHT        140
#define BORDER          5
#define I_WIDTH			40 		// icon button
#define I_HEIGHT        40
#define I_GAP			10
#define KERNEL          HEATMAP 		// needed odd value (like 1,3,5,...) 5 <- normal

#define SHOT_PNG		"shot.png"
#define SHOT2_PNG		"shot2.png"
#define EXIT_PNG		"exit.png"
#define CAPTURE_JPG		"capture.jpg"
#define WINDOW_JPG		"window.jpg"
#define PREVIEW_JPG		"preview.jpg"
#define DEBUG_PNG		"debug.png"
#define DEBUG2_PNG		"debug2.png"
#define CONN_PNG		"connect.png"
#define DISC_PNG		"disconnect.png"

/*
 *  DISPLAY SCREEN LAYOUT
 *  =====================
 *
 *  - Raspberry Pi 7" Touch Screen Display (800 x 600)
 *
 *   <------------ 640 --------------> <----- 160 ----->
 * 	+---------------------------------+-----------------+
 * 	|(0,0)                            | +-------------+ |    noise map
 * 	|                                 | |  NOISE MAP  | |    (120 x 90; 4:3)
 * 	|         CAMERA PREVIEW          | |             | |
 * 	|          (640 x 480)            | +-------------+ |
 * 	|                                 | +-------------+ |
 * 	|                                 | |  SHUTTER    | |
 * 	|                                 | +-------------+ |
 * 	|                                 | +-------------+ |
 * 	|                                 | |  EXIT       | |
 * 	|                        (640,480)| +-------------+ |
 * 	+---------------------------------+-----------------+
 * 	                                   <>              <> 20 (BORDER)
 *                                       <----120----->
 *
 *  SENSOR LAYOUT @CAMERA PREVIEW
 *  =============================
 *
 *  - SCREEN: width x height  = 640 x 380 (4:3)
 *  - SENSOR: width x height  = 160 x 240
 *
 *  <--------------------------640-------------------------->
 *  <----160------> width
 * 	+-------------+-------------+-------------+-------------+
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	|  SENSOR#0   |  SENSOR#1   |  SENSOR#2   |  SENSOR#3   |
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	+-------------+-------------+-------------+-------------+
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	|  SENSOR#4   |  SENSOR#5   |  SENSOR#6   |  SENSOR#7   |
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	|             |             |             |             |
 * 	+-------------+-------------+-------------+-------------+
 *
 *  HEATMAP CELL LAYOUT @SENSOR
 *  ===========================
 *
 *	- SENSOR: width x height = 160 x 240
 *	- CELL: width x height = 16 x 24 (10 x 10 matrix)
 *  <-16->
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 08 |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 01 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 02 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 03 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 04 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 05 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 06 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 07 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 08 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *  | 09 |    |    |    |    |    |    |    |    |    |
 *  +----+----+----+----+----+----+----+----+----+----+
 *
 */

#define	HIGH			1
#define LOW				0
#define GPIO_PIEZOE		20
#define GPIO_BUTTON		21
#define PUSH            1
#define TIMEOUT			1
#define THRESHOLD       50

typedef struct {
	int			id;								// sensor #id
	int     	p;								// peak value
	int     	n;                  			// frequency count
	int			o;								// opaque (0..255)
	int			v[NSAMPLE];						// sensor read value
	int 		f[NSAMPLE];						// fast fourier transform matrix mapping value
	double 		a[NSAMPLE];						// fast fourier transform matrix
	void* 		data;
} max4466_t;

typedef struct appdata {
	int 		verbose;						// verbose
	int 		network;						// network availability
	int 		nfail;							// #fail (peer server no response?)
	int			peak;							// noise peak
	int 		freq;							// noise frequency
	int         snap;                        	// please snapshot
	bool		snapshot;
	max4466_t	max4466[NSENSOR];
	int 		s[R_SENSOR][C_SENSOR];			// sensor map opaque
	int         h[R_HEATMAP][C_HEATMAP];		// heat map opaque

	Evas_Object *win;							// baseUI frame object
	Evas_Object *evas;							// windows child object
	Evas_Object *view;							// camera preview object
	Evas_Object *ctrl;							// camera control object
	Evas_Object *heatmap[R_HEATMAP][C_HEATMAP]; // heat map rectangle object
	Evas_Object *minimap[R_SENSOR][C_SENSOR]; 	// heat map rectangle object
	Evas_Object *overlay[R_SENSOR][C_SENSOR]; 	// heat map rectangle object
	Evas_Object *thumbnail;						// thumbnail
	Evas_Object *shot;							// button
	Evas_Object *shot2;							// button
	Evas_Object *conn;							// connection button
	Evas_Object *disc;							// disconnection button
	Evas_Object *debug;							// debug button
	Evas_Object *debug2;						// debug button
	Evas_Object *exit;							// exit

	Ecore_Idler *idler;
	Ecore_Timer *timer;
} appdata_t;



#ifdef __cplusplus
}
#endif

#endif /* __soundcamera_H__ */
