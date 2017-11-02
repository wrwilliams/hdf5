/*
 * This is the header for the S3 Communications module
 *
 * ***NOT A FILE DRIVER***
 */

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>



/*****************
 * PUBLIC MACROS *
 *****************/

/* "magic" numbers to identify structures
 */
#define S3COMMS_HRB_FL_MAGIC 0x7F5757UL

#define S3COMMS_HRB_MAGIC 0x6DCC84UL

#define S3COMMS_S3R_MAGIC 0x44d8d79

/* hexadecimal string of pre-computed sha256 checksum of the empty string 
 * hex(sha256sum(""))
 */
#define EMPTY_SHA256 \
"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

/* string length (plus null terminator)
 * "20170713T145903Z" (yyMMDD'T'hhmmss'_')
 */
#define ISO8601_SIZE 17

/* string length (plus null terminator) 
 * "Fri, 30 Jun 2017 20:41:55 GMT"
 */
#define RFC7231_SIZE 30

/*---------------------------------------------------------------------------
 *
 * Macro: ISO8601NOW()
 *
 * Purpose:
 *
 *     write "yyyyMMDD'T'hhmmss'Z'" (less single-quotes) to dest
 *     e.g., "20170630T204155Z"
 * 
 *     wrapper for strftime()
 * 
 *     not own code-block because we may want the output from strftime
 *
 * Programmer: Jacob Smith
 *             2017-07-??
 *
 *---------------------------------------------------------------------------
 */
#define ISO8601NOW(dest, now_gm) \
strftime((dest), ISO8601_SIZE, "%Y%m%dT%H%M%SZ", (now_gm))

/*---------------------------------------------------------------------------
 *
 * Macro: RFC7231NOW()
 *
 * Purpose:
 *
 *     write "Day, DD Mon yyyy hh:mm:ss GMT" to dest
 *     e.g., "Fri, 30 Jun 2017 20:41:55 GMT"
 * 
 *     wrapper for strftime()
 * 
 *     not own code-block because we may want the output from strftime
 *
 * Programmer: Jacob Smith
 *             2017-07-??
 *
 *---------------------------------------------------------------------------
 */
#define RFC7231NOW(dest, now_gm) \
strftime((dest), RFC7231_SIZE, "%a, %d %b %Y %H:%M:%S GMT", (now_gm))


/*---------------------------------------------------------------------------
 *
 * Macro: H5FD_S3COMMS_FORMAT_CREDENTIAL()
 *
 * Purpose:
 * 
 *     Format "S3 Credential" string from inputs, for AWS4.
 *
 *     Wrapper for sprintf().
 *
 *     _HAS NO ERROR-CHECKING FACILITIES_
 * 
 *     "<acces-key-id>/<date>/<aws-region>/<aws-service>/aws4_request"
 *     assuming that `dest` has adequate space.
 *
 *     ALl inputs must be null-terminated strings.
 *
 *     `access` should be the user's access key ID.
 *     `date` must be of format "yyyyMMDD".
 *     `region` should be relevant AWS region, i.e. "us-east-1".
 *     `service` should be "s3".
 *
 * Programmer: Jacob Smith
 *             2017-09-19
 *
 * Changes: None.
 *             
 *---------------------------------------------------------------------------
 */
#define H5FD_S3COMMS_FORMAT_CREDENTIAL(dest, access, iso8601_date, region, service) \
sprintf((dest), "%s/%s/%s/%s/aws4_request", (access), (iso8601_date), (region), (service))



/*********************
 * PUBLIC STRUCTURES *
 *********************/

/* sorting order enums 
 * used in `H5FD_s3comms_hrb_fl_next()` and `H5FD_s3comms_hrb_fl_first()`
 */
enum HRB_FL_ORD {
    HRB_FL_ORD_GIVEN, /* sort by order added, first to last */
    HRB_FL_ORD_SORTED, /* sort by lowername, least to greatest (via strcmp) */
};


/****************************************************************************
 *
 * Structure: hrb_fl_t_2
 *
 * Purpose:
 *
 *
 *     Maintain a set/list of HTTP Header fields, with field name and value.
 *
 *     Node for multiply doubley-linked list, each list for:
 *         * order added
 *         * lowercase sorted names
 *
 *     It is not allowed to have multiple nodes in a list with the same
 *     _lowercase_ `name`s.
 *     (i.e., name is case-insensitive for access and modification.)
 *
 *     All data (name, value, lowername, and cat 
 *         (concatnation %s: %s, name, value)
 *     are null-terminated strings allocated specifically for their node.
 *
 *
 *
 *     `magic` - "unique" idenfier number for the structure type
 *
 *     `name` - Case-meaningful name of the HTTP field.
 *              Given case is how it is supplied to networking code.
 *
 *     `value` - Case-meaningfule value of HTTP field.
 *
 *     `cat` - concatenated, null-terminated string of HTTP header line
 *                 sprintf("%s: %s", name, value)
 *
 *     `next`, `next_lower`, `prev`, and `prev_lower` are all pointers to
 *     other nodes (or NULL) within two double-linked lists.
 *         `next` and `prev` move in order given (next being added later).
 *         `*_lower` move in order sorted by lowername (next being "greater").
 *         
 *
 *
 * Programmer: Jacob Smith
 *             2017-09-22
 *
 ****************************************************************************
 */
typedef struct hrb_fl_t_2 {
    unsigned long      magic;
    char              *name;
    char              *value;
    char              *cat;
    char              *lowername;
    struct hrb_fl_t_2 *next;
    struct hrb_fl_t_2 *next_lower;
    struct hrb_fl_t_2 *prev;
    struct hrb_fl_t_2 *prev_lower;
} hrb_fl_t_2;


/****************************************************************************
 *
 * HTTP Request Buffer structure
 *
 * Structure for building HTTP requests at a medium-hign level, bundling
 * metadata (in its own components and `hrb_fl_t_2 "header lists"`) with
 * an optional body.
 *
 *
 *
 * magic (unsigned long int) :
 *
 *    "Magic" number confirming that this is an hrb_t structure and
 *    what operations are valid for it.
 *
 *    Must be S3COMMS_HRB_MAGIC to be valid.
 *
 * body (char *) :
 *
 *    Pointer to start of HTTP body.
 *
 *    Can be NULL, in which case it is treated as the empty string, "" .
 *
 * body_len (size_t) :
 *
 *    Number of bytes (characters) in `body`. 0 if empty or NULL `body`.
 *
 * first_header (hrb_fl_t_2 *) :
 *
 *     Pointer to first SORTED header node, if any.
 *
 * resource (char *) :
 *
 *     Pointer to resource URL string, e.g., "/folder/page.xhtml".
 *
 * verb (char *) :
 *
 *     Pointer to HTTP verb, e.g., "GET".
 *
 * version (char *) :
 *
 *     Pointer to start of string of HTTP version, e.g., "HTTP/1.1".
 *
 ****************************************************************************/
typedef struct {
    unsigned long  magic;
    char          *body;
    size_t         body_len;
    hrb_fl_t_2    *first_header;
    char          *resource;
    char          *verb;
    char          *version;
} hrb_t;


/*****************************************************************************
 *
 * Container for the elements of a URL
 * (nominally, http[s] url)
 *
 * Pointers to null-terminated strings allocated for this structure (or NULLs).
 *
 * If a component is NULL, it is either empty or absent from the URL.
 *
 * To reconstruct a URL:
 * SCHEME "://" HOST [ ":" PORT ] [ "/" PATH ] [ "?" QUERY ]
 * including bracketed elements if and only if its component is not NULL.
 *     Exception: if PATH is null and QUERY is not null, '/' should be included
 *
 *****************************************************************************/
typedef struct {
    char *scheme; /* required */
    char *host;   /* required */
    char *port;
    char *path;
    char *query;
} parsed_url_t;


/*****************************************************************************
 *
 * S3 request structure "handle"
 *
 * Holds persistent information for Amazon S3 requests.
 *
 * Instantiated through `H5FD_s3comms_s3r_open()`, copies data into self.
 *
 * Intended to be re-used.
 *
 * Cleaned up through `H5FD_s3comms_s3r_close()`
 *
 * _DO NOT_ share handle between threads.
 *
 *    curl easy handle `curlhandle` has undefined behavior
 *    if called to perform in multiple threads.
 *
 *
 *
 * magic (unsigned long int) :
 *
 *    "magic" number identifying this structure as unique type.
 *    MUST equal `S3R_MAGIC` to be valid.
 *
 * !!ABSENT!! curlcode (CURLcode) :
 *
 *    Store CURLcode output of calls to CURL, useful for making informed
 *    decisions in event of error (anything but CURLE_OK).
 *
 * curlhandle (CURL) :
 *
 *    Pointer to the curl_easy handle generated for the request.
 *
 * !!ABSENT!! httpcode (long int) :
 *
 *    Store HTTP response code (e.g. 404, 200, 206, 500) from "latest"
 *    interaction with server.
 *    (see https://curl.haxx.se/libcurl/c/CURLINFO_RESPONSE_CODE.html)
 *
 *    TODO: alternatively, just curl_easy_setopt(h, CURLOPT_FAILONERROR, 1L)
 *    and have programmer ask for this later?
 *    Fault would trigger CURLE_HTTP_RETURNED_ERROR during perform.
 *
 * httpverb (char *) :
 *
 *    Pointer to NULL-terminated string. HTTP verb,
 *    e.g. "GET", "HEAD", "PUT", etc.
 *
 *    Default is NULL, resulting in a "GET" request.
 *
 * purl (parsed_url_t *) :
 *
 *    Pointer to structure holding the elements of URL for file open.
 *
 *    e.g., "http://bucket.aws.com:8080/myfile.dat?q1=v1&q2=v2"
 *    parsed into...
 *    {   scheme: "http"
 *        host:   "bucket.aws.com"
 *        port:   "8080"
 *        path:   "myfile.dat"
 *        query:  "q1=v1&q2=v2"
 *    }
 *
 *    Cannot be NULL during read.
 *
 * region (char *) :
 *
 *    Pointer to NULL-terminated string, specifying S3 "region",
 *    e.g., "us-east-1".
 *
 *    Required to authenticate.
 *
 * secret_id (char *) :
 *
 *    Pointer to NULL-terminated string for "secret" access id to S3 resource.
 *
 *    Requred to authenticate.
 *
 * signing_key (unsigned char *) :
 *
 *    Pointer to `SHA256_DIGEST_LENGTH`-long string for "re-usable" signing
 *    key, generated via
 *    `HMAC-SHA256(HMAC-SHA256(HMAC-SHA256(HMAC-SHA256("AWS4<secret_key>",
 *        "<yyyyMMDD"), "<aws-region>"), "<aws-service>"), "aws4_request")`
 *    which may be re-used for several (up to seven (7)) days from creation?
 *
 *    Requred to authenticate.
 *
 *
 *****************************************************************************/
typedef struct {
    unsigned long int  magic;
    CURL              *curlhandle;
    size_t             filesize;
    char              *httpverb;
    parsed_url_t      *purl;
    char              *region;
    char              *secret_id;
    unsigned char     *signing_key;
} s3r_t;



/*******************************************
 * DECLARATION OF HTTP FIELD LIST ROUTINES *
 *******************************************/

herr_t H5FD_s3comms_hrb_fl_destroy(hrb_fl_t_2 *L);

hrb_fl_t_2 * H5FD_s3comms_hrb_fl_first(hrb_fl_t_2      *L,
                                       enum HRB_FL_ORD  ord);

hrb_fl_t_2 * H5FD_s3comms_hrb_fl_next(hrb_fl_t_2      *L, 
                                      enum HRB_FL_ORD  ord);

hrb_fl_t_2 * H5FD_s3comms_hrb_fl_set(hrb_fl_t_2 *L,
                                     const char *name,
                                     const char *value);



/********************************************************
 * DECLARATION OF HTTP REQUEST|RESPONSE BUFFER ROUTINES *
 ********************************************************/

herr_t H5FD_s3comms_hrb_destroy(hrb_t *buf);

hrb_t * H5FD_s3comms_hrb_init_request(const char *verb,
                                      const char *resource,
                                      const char *host);



/*************************************
 * DECLARATION OF S3REQUEST ROUTINES *
 *************************************/

herr_t H5FD_s3comms_s3r_close(s3r_t *handle);

herr_t H5FD_s3comms_s3r_getsize(s3r_t *handle);

s3r_t * H5FD_s3comms_s3r_open(const char          url[],
                              const char          region[],
                              const char          id[],
                              const unsigned char signing_key[]);

herr_t H5FD_s3comms_s3r_read(s3r_t  *handle,
                             size_t  offset,
                             size_t  len,
                             void   *dest);



/*********************************
 * DECLARATION OF OTHER ROUTINES *
 *********************************/

struct tm * gmnow(void);

herr_t H5FD_s3comms_aws_canonical_request(char  *canonical_request_dest,
                                          char  *signed_headers_dest,
                                          hrb_t *http_request);

herr_t H5FD_s3comms_bytes_to_hex(char                *dest,
                                 const unsigned char *msg,
                                 size_t               msg_len,
                                 hbool_t              lowercase);

herr_t H5FD_s3comms_free_purl(parsed_url_t *purl);

herr_t H5FD_s3comms_HMAC_SHA256(const unsigned char *key,
                                size_t               key_len,
                                const char          *msg,
                                size_t               msg_len,
                                char                *dest);

herr_t H5FD_s3comms_nlowercase(char       *dest,
                               const char *s,
                               size_t      len);

herr_t H5FD_s3comms_parse_url(const char    *str, 
                              parsed_url_t **purl);

herr_t H5FD_s3comms_percent_encode_char(char               *repr,
                                        const unsigned char c,
                                        size_t             *repr_len);

herr_t H5FD_s3comms_signing_key(unsigned char *md,
                                const char    *secret,
                                const char    *region,
                                const char    *iso8601now);

herr_t H5FD_s3comms_tostringtosign(char       *dest,
                                   const char *req_str,
                                   const char *now,
                                   const char *region);

herr_t H5FD_s3comms_trim(char   *dest,
                         char   *s,
                         size_t  s_len,
                         size_t *n_written);

herr_t H5FD_s3comms_uriencode(char       *dest,
                              const char *s,
                              size_t      s_len,
                              hbool_t     encode_slash,
                              size_t     *n_written);


