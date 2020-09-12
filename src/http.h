#ifndef HTTP_H
#define HTTP_H


// A buffer object with data, and a length
typedef struct {
    char *data;
    size_t length;

} Buffer;


/**
 * Perform an HTTP 1.0 query to a given host and page and port number.
 * host is a hostname and page is a path on the remote server. The query
 * will attempt to retrievev content in the given byte range.
 * User is responsible for freeing the memory.
 * 
 * @param host - The host name e.g. www.canterbury.ac.nz
 * @param page - e.g. /index.html
 * @param range - Byte range e.g. 0-500. NOTE: A server may not respect this
 * @param port - e.g. 80
 * @return Buffer - Pointer to a buffer holding response data from query
 *                  NULL is returned on failure.
 */
Buffer* http_query(char *host, char *page, const char *range, int port);


/**
 * Separate the content from the header of an http request.
 * NOTE: returned string is an offset into the response, so
 * should not be freed by the user. Do not copy the data.
 * @param response - Buffer containing the HTTP response to separate 
 *                   content from
 * @return string response or NULL on failure (buffer is not HTTP response)
 */
char* http_get_content(Buffer *response);


/**
 * Splits an HTTP url into host, page. On success, calls http_query
 * to execute the query against the url. 
 * @param url - Webpage url e.g. learn.canterbury.ac.nz/profile
 * @param range - The desired byte range of data to retrieve from the page
 * @return Buffer pointer holding raw string data or NULL on failure
 */
Buffer *http_url(const char *url, const char *range);


/**
 * Free a buffer
 * @param buffer - Pointer to a buffer to free
 */ 
inline static void buffer_free(Buffer *buffer) {
    free(buffer->data);
    free(buffer);
}


/**
 * Makes a HEAD request to a given URL and gets the content length
 * maxByteSize is set from this, and number of split downloads determined
 * @param url   The URL of the resource to download
 * @param threads   The number of threads to be used for the download
 * @return int  The number of downloads needed satisfying maxByteSize
 *              to download the resource
 */
int get_num_tasks(char *url, int threads);

int max_chunk_size; // The maximum size in bytes of a chunk to download

int get_max_chunk_size(void);

#endif
