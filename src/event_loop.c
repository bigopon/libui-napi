#include <stdbool.h>
#include "event-loop.h"
#include "napi_utils.h"
#include "app.h"

static const char* MODULE = "EventLoop";

bool _Atomic running;

bool _Atomic mainThreadStillWaitingGuiEvents;

static uv_mutex_t mainThreadWaitingGuiEvents;
static uv_mutex_t mainThreadAwakenFromBackground;
static uv_prepare_t mainThreadAwakenPhase;
static uv_async_t keepAliveTimer;

static uv_thread_t thread;
static uv_timer_t redrawTimer;
static uv_timer_t closeTimer;

/*
   This function is executed in the
   background thread and is responsible to continuosly polling
   the node backend for pending events.

   When pending node events are found, the native GUI
   event loop is wake up by calling uiLoopWakeup().
*/
static void backgroundNodeEventsPoller(void *arg) {
	while (running) {
		//DEBUG("--- wait mainThreadWaitingGuiEvents.\n");

		// wait for the main thread
		// to be blocked waiting for GUI events
		uv_mutex_lock(&mainThreadWaitingGuiEvents);

		// immediately release the lock
		uv_mutex_unlock(&mainThreadWaitingGuiEvents);

		int pendingEvents = 1;
		int timeout = uv_backend_timeout(uv_default_loop());
		//DEBUG_F("--- uv_backend_timeout == %d\n", timeout);

		if (timeout != 0) {
			do {

				//DEBUG_F("--- entering wait with timeout %d\n", timeout);
				/* wait for pending events*/
				pendingEvents = waitForNodeEvents(uv_default_loop(), timeout);

				if (pendingEvents == 0 && timeout > 0) {
					// timers
					pendingEvents = 1;
				}
			} while (pendingEvents == -1 && errno == EINTR);
		}

		//DEBUG_F("--- pendingEvents == %d\n", pendingEvents);

		if (running && mainThreadStillWaitingGuiEvents && pendingEvents > 0) {
			//DEBUG("--- wake up main thread\n");
			// this allow the background thread
			// to wait for the main thread to complete
			// running node callbacks
			uiLoopWakeup();
		}

		if (running && pendingEvents > 0) {
			// wait for the main thread to complete
			// its awaken phase.
			//DEBUG("--- mainThreadAwakenFromBackground locking.\n");
			uv_mutex_lock(&mainThreadAwakenFromBackground);
			//DEBUG("--- mainThreadAwakenFromBackground locked.\n");

			// immediately release the lock
			uv_mutex_unlock(&mainThreadAwakenFromBackground);
		}
	}
	//DEBUG("--- Background terminating.\n");
}


static void redraw(uv_timer_t *handle);

static void uv_awaken_cb(uv_prepare_t *handle) {
	uv_prepare_stop(&mainThreadAwakenPhase);

	if (!running) {
		return;
	}

	//DEBUG("+++ mainThreadAwakenFromBackground unlocking.\n");
	uv_mutex_unlock(&mainThreadAwakenFromBackground);
	//DEBUG("+++ mainThreadAwakenFromBackground unlocked.\n");

	// schedule another call to redraw as soon as possible
	// how to find a correct amount of time to schedule next call?
	//.because too long and UI is not responsive, too short and node
	// become really slow
	uv_timer_start(&redrawTimer, redraw, 10, 0);
}

/*
	This function run all pending native GUI event in the loop
	using libui calls.

	It first do a blocking call to uiMainStep that
	wait for pending GUI events.
	The function also exit when there are pending node
	events, because uiLoopWakeup function posts a GUI event
	from the background thread for this purpose.
 */
static void redraw(uv_timer_t *handle) {
	if (!running) {
		return;
	}
	uv_timer_stop(handle);



	// signal the background poller thread
	// that the main one is about to enter
	// the blocking call to wait for GUI events.
	uv_mutex_unlock(&mainThreadWaitingGuiEvents);
	mainThreadStillWaitingGuiEvents = true;

	//DEBUG("+++ locking mainThreadAwakenFromBackground\n");
	uv_mutex_lock(&mainThreadAwakenFromBackground);
	//DEBUG("+++ locked mainThreadAwakenFromBackground\n");

	/* Blocking call that wait for a node or GUI event pending */
	//DEBUG("+++ blocking GUI\n");
	uiMainStep(true);
	mainThreadStillWaitingGuiEvents = false;
	uv_mutex_lock(&mainThreadWaitingGuiEvents);
	//DEBUG("+++ mainThreadWaitingGuiEvents locked.\n");

	/* dequeue and run every event pending */
	while (uiEventsPending()) {
		running = uiMainStep(false);
		//DEBUG("+++ other GUI event dequeued.\n");
	}
	//DEBUG("+++ all GUI events dequeued.\n");

	// uv_mutex_unlock(&mainThreadAwakenFromBackground);

	// uv_timer_start(redrawTimer, redraw, 100, 0);

	if (running) {
		uv_prepare_start(&mainThreadAwakenPhase, uv_awaken_cb);
		//DEBUG("+++ prepare handler started.\n");
	}
}

/* This function start the event loop and exit immediately */
static void stopAsync(uv_timer_t *handle) {
	printf("stopAsync entern\n");
	if (!running) {
		return;
	}

	running = false;

	//DEBUG("stopAsync\n");

	/* stop redraw handler */
	uv_timer_stop(&redrawTimer);
	uv_close((uv_handle_t *) &redrawTimer, NULL);
	//DEBUG("redrawTimer\n");

	uv_timer_stop(handle);
	//DEBUG("handle\n");
	uv_close((uv_handle_t *)handle, NULL);

	if (uv_mutex_trylock(&mainThreadWaitingGuiEvents)) {
		uv_mutex_unlock(&mainThreadWaitingGuiEvents);
	}

	if (uv_mutex_trylock(&mainThreadAwakenFromBackground)) {
		uv_mutex_unlock(&mainThreadAwakenFromBackground);
	}

	/* await for the background thread to finish */
	//DEBUG("uv_thread_join wait\n");
	uv_async_send(&keepAliveTimer);
	uv_thread_join(&thread);
	//DEBUG("uv_thread_join done\n");

	uv_mutex_destroy(&mainThreadWaitingGuiEvents);
	//DEBUG("uv_mutex_destroy mainThreadWaitingGuiEvents done\n");

	// TODO: improve synchronization
	// some examples crash while running this
	// uv_mutex_destroy(&mainThreadAwakenFromBackground);
	//DEBUG("uv_mutex_destroy mainThreadAwakenFromBackground done\n");

	/* stop keep alive timer */
	uv_close((uv_handle_t *)&keepAliveTimer, NULL);

	//DEBUG("uv_close keepAliveTimer done\n");

	uv_close((uv_handle_t *)&mainThreadAwakenPhase, NULL);

	/* quit libui event loop */
	uiQuit();
}

/* This function start the event loop and exit immediately */
void startLoop() {
	/* if the loop is already running, this is a noop */
	if (running) {
		return;
	}

	running = true;
	mainThreadStillWaitingGuiEvents = false;
	/* init libui event loop */
	uiMainSteps();
	//DEBUG("uiMainSteps...\n");

	// this is used to signal the background thread
	// that the main one is entering a blocking call
	// waiting for GUI events.
	uv_mutex_init(&mainThreadWaitingGuiEvents);

	// lock the mutex to signal that main
	// thread is not yet blocked waiting
	// for GUI events
	uv_mutex_lock(&mainThreadWaitingGuiEvents);

	// this is used to signal the background thread
	// when all pending callback of current tick are
	// called.
	uv_mutex_init(&mainThreadAwakenFromBackground);

	uv_prepare_init(uv_default_loop(), &mainThreadAwakenPhase);

	/* start the background thread that check for node evnts pending */
	uv_thread_create(&thread, backgroundNodeEventsPoller, NULL);
	//DEBUG("thread...\n");

	// Add dummy handle for libuv, otherwise libuv would quit when there is
	// nothing to do.
	uv_async_init(uv_default_loop(), &keepAliveTimer, NULL);

	/* start redraw timer */
	uv_timer_init(uv_default_loop(), &redrawTimer);
	uv_timer_start(&redrawTimer, redraw, 1, 0);

	//DEBUG("redrawTimer...\n");
}



/* This function start the event loop and exit immediately */
void stopLoop() {

	uv_timer_init(uv_default_loop(), &closeTimer);
	uv_timer_start(&closeTimer, stopAsync, 1, 0);
	printf("stopLoop sync done\n");

}

LIBUI_FUNCTION(start) {
	startLoop();
	return NULL;
}

LIBUI_FUNCTION(stop) {
	destroy_all_children(env, visible_windows);
	clear_children(env, visible_windows);
	visible_windows = NULL;
	stopLoop();
	return NULL;
}

// this function signal make background
// to stop awaiting node events, allowing it
// to update the list of handles it's
// awaiting for.
LIBUI_FUNCTION(wakeupBackgroundThread) {
	uv_async_send(&keepAliveTimer);
	return NULL;
}

napi_value _libui_init_event_loop (napi_env env, napi_value exports) {
	DEFINE_MODULE();
	LIBUI_EXPORT(wakeupBackgroundThread);
	LIBUI_EXPORT(start);
	LIBUI_EXPORT(stop);
	return module;
}
