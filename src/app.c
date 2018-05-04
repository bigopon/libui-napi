#include <string.h>
#include <ui.h>
#include <stdlib.h>
#include <stdio.h>
#include "napi_utils.h"
#include "control.h"
#include "events.h"
#include "event-loop.h"

static const char* MODULE = "App";

struct children_list* visible_windows = NULL;

static int onShouldQuit_cb(void *data) {
	struct event_t *event = (struct event_t *)data;
	fire_event(event);
	return 0;
}

LIBUI_FUNCTION(onShouldQuit) {
	INIT_ARGS(1);
	ARG_CB_REF(cb_ref, 0);

	struct event_t *event = create_event(env, cb_ref, "onShouldQuit");
	if (event == NULL) {
		return NULL;
	}

	uiOnShouldQuit(onShouldQuit_cb, event);

	return NULL;
}

LIBUI_FUNCTION(init) {
	uiInitOptions o;
	memset(&o, 0, sizeof(uiInitOptions));
	const char *err = uiInit(&o);
	if (err != NULL) {
		napi_throw_error(env, NULL, err);
		uiFreeInitError(err);
	}
	controls_map = ctrl_map_create(0, 1);
	visible_windows = create_children_list();
	return NULL;
}

napi_value _libui_init_core (napi_env env, napi_value exports) {
	DEFINE_MODULE();
	LIBUI_EXPORT(onShouldQuit);
	LIBUI_EXPORT(init);
	return module;
}