#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#include "http.h"

#define BUF_SIZE 1024


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

int init_socket(char *host, int port){

    int socket_id = socket(AF_INET, SOCK_STREAM, 0);     //create client socket
    struct addrinfo* server_address = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));     // set 0
    hints.ai_family = AF_INET;                      // IPv4
    hints.ai_socktype = SOCK_STREAM;                // TCP
    char port_num[16];
    sprintf(port_num, "%d", port);                  // convert int to char* for matching getaddrinfo
    if (getaddrinfo(host, port_num, &hints, &server_address) != 0) {
        perror("Error: getaddrinfo");
        exit(1);
    }
                                                    //connect to server
    if (connect(socket_id, server_address->ai_addr, server_address->ai_addrlen) == -1) {
        perror("Error connecting to server");
        exit(2);
    }

    freeaddrinfo(server_address);                    //free server_address
    return socket_id;
}

Buffer* http_query(char *host, char *page, const char *range, int port) {

    int socket_id = init_socket(host, port);                        //init socket
    Buffer* response_data = malloc(sizeof(Buffer));                 //init buffer for receiving reponse data
    response_data->data = (char*)calloc(BUF_SIZE,sizeof(char));
    response_data->length = 0;

    char request[100];
    char head[100] = "GET /%s HTTP/1.0\r\nHost: %s\r\nRange: bytes=";
    strcat(head, range);                                         //add the related download range
    strcat(head, "\r\nUser-Agent: getter\r\n\r\n");
    //printf("%s %s %s\n", host, page, range);
    sprintf(request, head, page, host);                        //append host and page into HTTP request
    if (write(socket_id, request, strlen(request)) == -1) perror("Error making request to server.");
                                                             // send the HTTP request to server
    char buffer[BUF_SIZE];
    int num_bytes = 1;
    if (num_bytes == -1)  perror("Error reading from server.");

    while (num_bytes > 0) {                           //realloc the memory size when incoming data size over length
        num_bytes = read(socket_id, buffer, BUF_SIZE);
        response_data->data = realloc(response_data->data, response_data->length + num_bytes);
        memcpy(response_data->data + response_data->length, buffer, num_bytes);
        response_data->length += num_bytes;
    }

    close(socket_id);                           //close socket after everything
    return response_data;
}


/**
 * Separate the content from the header of an http request.
 * NOTE: returned string is an offset into the response, so
 * should not be freed by the user. Do not copy the data.
 * @param response - Buffer containing the HTTP response to separate
 *                   content from
 * @return string response or NULL on failure (buffer is not HTTP response)
 */
char* http_get_content(Buffer *response) {

    char* header_end = strstr(response->data, "\r\n\r\n");

    if (header_end) {
        return header_end + 4;
    }
    else {
        return response->data;
    }
}


/**
 * Splits an HTTP url into host, page. On success, calls http_query
 * to execute the query against the url.
 * @param url - Webpage url e.g. learn.canterbury.ac.nz/profile
 * @param range - The desired byte range of data to retrieve from the page
 * @return Buffer pointer holding raw string data or NULL on failure
 */
Buffer *http_url(const char *url, const char *range) {
    char host[BUF_SIZE];
    strncpy(host, url, BUF_SIZE);

    char *page = strstr(host, "/");

    if (page) {
        page[0] = '\0';
        ++page;
        return http_query(host, page, range, 80);
    }
    else {
        fprintf(stderr, "could not split url into host/page %s\n", url);
        return NULL;
    }
}


/**
 * Makes a HEAD request to a given URL and gets the content length
 * Then determines max_chunk_size and number of split downloads needed
 * @param url   The URL of the resource to download
 * @param threads   The number of threads to be used for the download
 * @return int  The number of downloads needed satisfying max_chunk_size
 *              to download the resource
 */

int socket_2(char *url){

    char host[BUF_SIZE];                   //split url to host and page by the first occurrence of "/"
    strncpy(host, url, BUF_SIZE);
    char *page = strstr(host, "/");

    page[0] = '\0';
    ++page;

    int socket_id = socket(AF_INET, SOCK_STREAM, 0);      //create client socket
    struct addrinfo* server_address = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));           // set 0
    hints.ai_family = AF_INET;                            // IPv4
    hints.ai_socktype = SOCK_STREAM;                      // TCP
    if (getaddrinfo(host, "80", &hints, &server_address) != 0) perror("Error: getaddrinfo");
    if (connect(socket_id, server_address->ai_addr, server_address->ai_addrlen) == -1) perror("Error connecting to server");

    freeaddrinfo(server_address);                       //free server_address

    return socket_id;
}

int get_num_tasks(char *url, int threads) {
    char host[BUF_SIZE];                     //split url to host and page by the first occurrence of "/"
    strncpy(host, url, BUF_SIZE);
    char *page = strstr(host, "/");
    page[0] = '\0';
    ++page;

    int socket_id = socket_2(url);
    char* request = (char*)calloc(BUF_SIZE, sizeof(char));
    sprintf(request, "HEAD /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: getter\r\n\r\n", page, host);
    if (write(socket_id, request, strlen(request)) == -1) perror("Error making content length request to server.");

    char* buffer = malloc(BUF_SIZE);
    int num_bytes = read(socket_id, buffer, BUF_SIZE);
    if (num_bytes == -1) perror("Error reading content length from server.");

    char *result = strstr(buffer, "Content-Length:");    //find the start index of content length
    char content_length[20] = "\0";
    if (result != NULL){
        int index = result - buffer + strlen("Content-Length: ");
        int i = 0;
        while (isdigit(buffer[index])){         //while next char is digit, append it into array
            content_length[i] = buffer[index];
            i++;
            index++;
        }
    }
    int content_length_int = atoi(content_length);     // convert content_length char array to int
    max_chunk_size = content_length_int / threads + 1;
    //printf("content_length: %d  max_chunk_size %d\n", content_length_int, max_chunk_size);

    free(request);
    free(buffer);
    close(socket_id);

    return threads;
}

int get_max_chunk_size() {
    return max_chunk_size;
}

