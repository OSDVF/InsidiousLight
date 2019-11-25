#pragma once
#include "esp_https_server.h"
/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192
/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE   (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"

class ZarovkaWebServer
{
    private:
    static inline char _scratch[SCRATCH_BUFSIZE];
    static constexpr const char *TAG = "Zarovka:Https";
    /* An HTTP GET handler */
    static esp_err_t pageGetHandler(httpd_req_t *req);
    static esp_err_t uploadPostHandler(httpd_req_t *req);
    static esp_err_t deletePostHandler(httpd_req_t *req);
    static esp_err_t replyWithDirectoryListing(httpd_req_t *req, char *dirpath);
    static const char* _getPathFromUri(char *dest, const char *uri, size_t destsize);
    static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);
    static inline const char *_basePath;
    static inline size_t _basePathLen;
    public:
    static httpd_handle_t Start(const char* baseFilesystemPath);
    static void Stop(httpd_handle_t server);
};