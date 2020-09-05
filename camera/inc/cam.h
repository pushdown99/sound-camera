#ifndef CAM_H_
#define CAM_H_

typedef struct {
	camera_h	ch;
} camera_t;

extern camera_t*	cam;

void capCAM  (void *data);
int  initCAM (void *data);
void termCAM (void *data);

#endif /* CAM_H_ */
