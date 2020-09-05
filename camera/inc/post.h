
#ifndef POST_H_
#define POST_H_

#define FFT_POST_URL		"http://debian.tric.kr:9900/fft"
#define SENSOR_POST_URL		"http://debian.tric.kr:9900/data"
#define IMAGE_UPLOAD_URL	"http://debian.tric.kr:9900/upload"
#define THINGSPARK_URL		"https://api.thingspark.kr/channels/entrys"
#define THINGSPARK_KEY      "JmEeoADpTuDg2FZB"

void initCURL 		(void *data);
void termCURL 		(void *data);
void curlPost		(void *data, char *url);
void curlPostData 	(void *data, char *url, char *field);
void curlPostFile	(void *data, char *url, char *path);

#endif /* POST_H_ */
