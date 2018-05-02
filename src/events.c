#include "events.h"

napi_value fire_event(struct event_t *event) {
	napi_status status;
	napi_env env = event->env;

	napi_value resource_object;
	status = napi_create_object(env, &resource_object);
	CHECK_STATUS_UNCAUGHT(status, napi_create_object, NULL);

	napi_callback_scope scope;
	status = napi_open_callback_scope(
		env,
		resource_object,
		event->context,
		&scope
	);
	CHECK_STATUS_UNCAUGHT(status, napi_open_callback_scope, NULL);

	napi_value cb;
	status = napi_get_reference_value(env, event->cb_ref, &cb);
	CHECK_STATUS_UNCAUGHT(status, napi_get_reference_value, NULL);

	napi_value result;
	status = napi_make_callback(
		env,
		event->context,
		resource_object,
		cb,
		0,
		NULL,
		&result
	);

	if (status == napi_pending_exception) {
		napi_value last_exception;
		napi_get_and_clear_last_exception(env, &last_exception);
		napi_fatal_exception(env, last_exception);
		return NULL;
	}

	CHECK_STATUS_UNCAUGHT(status, napi_make_callback, NULL);

	status = napi_close_callback_scope(env, scope);
	CHECK_STATUS_UNCAUGHT(status, napi_close_callback_scope, NULL);

	return result;
}


struct event_t *create_event(napi_env env, napi_ref cb_ref, const char *name) {
	napi_status status;

	napi_value async_resource_name;
	status = napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &async_resource_name);
	CHECK_STATUS_THROW(status, napi_create_string_utf8);

	napi_async_context async_context;
	status = napi_async_init(env, NULL, async_resource_name, &async_context);
	CHECK_STATUS_THROW(status, napi_async_init);

	struct event_t *args = calloc(1, sizeof(struct event_t));
	args->cb_ref = cb_ref;
	args->env = env;
	args->context = async_context;

	return args;
}

napi_value clear_event(struct event_t *event) {
	uint32_t new_ref_count;
	napi_env env = event->env;
	napi_status status = napi_reference_unref(
		env,
		event->cb_ref,
		&new_ref_count
	);
	CHECK_STATUS_THROW(status, napi_reference_unref);

	free(event);
	return NULL;
}


static struct events_node *new_events_node(struct event_t *event) {
	struct events_node *new_node = malloc(sizeof(struct events_node));
	new_node->next = NULL;
	new_node->event = event;
	return new_node;
}

void install_event(struct events_list *events, struct event_t *event) {
	if (events->events_head == NULL) {
		// First event for this control
		struct events_node *new_node = new_events_node(event);
		events->events_head = new_node;
		events->events_tail = new_node;
		return;
	}

	// TODO: we need to remove existing events_node for the same event
	// and support NULL event to only remove event.

	// Control already has other events. Append to tail
	struct events_node *new_node = new_events_node(event);
	events->events_tail->next = new_node;

	// set this node as the new tail
	events->events_tail = new_node;
}


void clear_all_events(struct events_list *events) {
	if (events->events_head == NULL) {
		// This control has no events
		return;
	}

	struct events_node *node = events->events_head;
	struct events_node *node_to_free;

	while (node != NULL) {
		clear_event(node->event);
		node_to_free = node;
		node = node->next;
		free(node_to_free);
	}

	events->events_head = NULL;
	events->events_tail = NULL;
}