#pragma once

#if defined(__has_include)
#if __has_include("wifi_config.local.h")
#include "wifi_config.local.h"
#endif
#endif

#ifndef USE_HTTPS
#define USE_HTTPS 0
#endif
#ifndef HOST
#define HOST ""
#endif
#ifndef PORT
#define PORT 80
#endif
#ifndef BASE_URL_PATH
#define BASE_URL_PATH "/"
#endif
#ifndef LIST_FILE_REMOTE
#define LIST_FILE_REMOTE ""
#endif
#ifndef LIST_FILE_LOCAL
#define LIST_FILE_LOCAL ""
#endif

#define STATIC_ASSERT(cond, name) typedef char static_assert_##name[(cond) ? 1 : -1]

STATIC_ASSERT(sizeof(HOST) > 1, host_must_be_set_in_wifi_config_local_h);
STATIC_ASSERT(PORT != 0, port_must_be_set_in_wifi_config_local_h);
STATIC_ASSERT(sizeof(BASE_URL_PATH) > 1, base_url_path_must_be_set_in_wifi_config_local_h);
STATIC_ASSERT(sizeof(LIST_FILE_REMOTE) > 1, list_file_remote_must_be_set_in_wifi_config_local_h);
STATIC_ASSERT(sizeof(LIST_FILE_LOCAL) > 1, list_file_local_must_be_set_in_wifi_config_local_h);

#undef STATIC_ASSERT
