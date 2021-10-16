#ifndef _REQUESTS_
#define _REQUESTS_

#include "nlohmann/json.hpp"

using json = nlohmann::json;

// computes and returns a GET request string (query_params, token
// and cookies can be set to NULL if not needed)
char *compute_get_request(const char *host, const char *url, const char *cookie, const char *token);

// computes and returns a POST request string (cookies and token can be NULL if not needed)
char *compute_post_request(const char *host, const char *url, const char *content_type, json &j,
                           const char *cookie, const char *token);

// computes and returns a DELETE request string (cookies and token can be NULL if not needed)
char *compute_delete_request(const char *host, const char *url, const char *cookie,
                             const char *token);

#endif
