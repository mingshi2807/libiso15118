// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 Pionix GmbH and Contributors to EVerest
#ifndef ISO15118_C_API_H
#define ISO15118_C_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iso15118_session_t iso15118_session_t;

typedef void (*iso15118_event_fn)(void *userdata, const char *json_event);

iso15118_session_t *iso15118_session_create(const char *config_json);
void                iso15118_session_destroy(iso15118_session_t *s);
int                 iso15118_session_poll(iso15118_session_t *s);
void                iso15118_session_push_event(iso15118_session_t *s,
                                                 const char *event_json);
void                iso15118_session_set_callback(iso15118_session_t *s,
                                                   iso15118_event_fn fn,
                                                   void *userdata);
void                iso15118_session_close(iso15118_session_t *s);
const char         *iso15118_last_error(void);

#ifdef __cplusplus
}
#endif
#endif
