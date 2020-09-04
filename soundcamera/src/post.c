#include "soundcam.h"

void initCURL (void *data)
{
	curl_global_init (CURL_GLOBAL_ALL);
}

void termCURL (void *data)
{
	curl_global_cleanup ();
}

void curlPost(void *data, char *url)
{
	appdata_t *ad = (appdata_t*)data;
	CURL* 		curl;
	CURLcode 	code;

	if(ad->network == 0) return;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		code = curl_easy_perform(curl);
		if(code != CURLE_OK) {
			ad->nfail += 1;
			if(ad->verbose)
				_E("curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
		}
		curl_easy_cleanup(curl);
	}
}


void curlPostData(void *data, char *url, char *field)
{
	appdata_t *ad = (appdata_t*)data;
	CURL* 		curl;
	CURLcode 	code;

	if(ad->network == 0) return;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, field);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		code = curl_easy_perform(curl);
		if(code != CURLE_OK) {
			ad->nfail += 1;
			if(ad->verbose)
				_E("curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
		}
		curl_easy_cleanup(curl);
	}
}

void curlPostFile(void *data, char *url, char *path)
{
	appdata_t *ad = (appdata_t*)data;
	CURL* 		curl = NULL;
	CURLcode 	code;

	if(ad->network == 0) return;

	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	struct curl_slist *headerlist = NULL;
	static const char buf[] = "Expect:";

	curl_formadd(&formpost,	&lastptr, CURLFORM_COPYNAME, "sampleFile", CURLFORM_FILE, 		  path,   CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit",     CURLFORM_COPYCONTENTS, "send", CURLFORM_END);

	curl = curl_easy_init();
	headerlist = curl_slist_append(headerlist, buf);
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		code = curl_easy_perform(curl);
		if(code != CURLE_OK) {
			ad->nfail += 1;
			if(ad->verbose)
				_E("curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
		}
		curl_easy_cleanup(curl);
	    curl_formfree(formpost);
	    curl_slist_free_all(headerlist);
	}
}
