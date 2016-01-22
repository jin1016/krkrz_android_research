
#include <jni.h>
#include <errno.h>
#include <dirent.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

/**
 * Our saved state data.
 */
struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine {
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

    int animating;
	/*
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
	*/
    int32_t width;
    int32_t height;
    struct saved_state state;

	void OnTouchDown( float x, float y, float cx, float cy, int32_t id, float pressure, int32_t meta ) {
		LOGI( "Touch Down x : %f, y: %f, cx : %f, cy : %f, id : %d, meta : %d\n", x, y, cx, cy, id, meta );
	}
	void OnTouchMove( float x, float y, float cx, float cy, int32_t id, float pressure,int32_t meta ) {
		LOGI( "Touch Move x : %f, y: %f, cx : %f, cy : %f, id : %d, meta : %d\n", x, y, cx, cy, id, meta );
	}
	void OnTouchUp( float x, float y, float cx, float cy, int32_t id, float pressure,int32_t meta ) {
		LOGI( "Touch Up x : %f, y: %f, cx : %f, cy : %f, id : %d, meta : %d\n", x, y, cx, cy, id, meta );
	}
	/*
	void OnClick(tjs_int x, tjs_int y);
	void OnDoubleClick(tjs_int x, tjs_int y);
	void OnMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags);
	void OnMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags);
	void OnMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags);
	void OnMouseWheel();
	*/
};
// 以下のようにして、ttfファイルを取得すれば、フォントを使えそうではある。
// ただ、全部開いてフェイス名キャッシュして～となると、少し時間かかりそう。
// アーカイブ内にフォント入れて、それを読んで表示するのが確実か。
static void print_font_files() {
	DIR *dir;
	if( (dir=opendir("/system/fonts")) != NULL) {
		struct dirent *dp;
		for( dp = readdir(dir); dp != NULL; dp = readdir(dir) ) {
			if( dp->d_type == DT_REG ) {
				LOGI( "%s\n", dp->d_name );
			}
		}
		closedir(dir);
	}
}
/**
 * 
 */
static int engine_init_display(struct engine* engine) {

	engine->width = ANativeWindow_getWidth( engine->app->window );
    engine->height = ANativeWindow_getHeight( engine->app->window );
	int32_t format = ANativeWindow_getFormat( engine->app->window );
	LOGI( "original width:%d\n", engine->width );
	LOGI( "original height:%d\n", engine->height );
	LOGI( "original format:%d\n", format );
	engine->width = 640;
    engine->height = 480;
    ANativeWindow_setBuffersGeometry(engine->app->window, engine->width, engine->height, WINDOW_FORMAT_RGBX_8888);
    engine->state.angle = 0;

	//print_font_files();
    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
	/*
    if (engine->display == NULL) {
        // No display.
        return;
    }
	*/

	ARect dirty;
	dirty.left = 0;
	dirty.top = 0;
	dirty.right = engine->width;
	dirty.bottom = engine->height;
	ANativeWindow_Buffer buffer;
	ANativeWindow_lock( engine->app->window, &buffer, &dirty );
	unsigned char* bits = (unsigned char*)buffer.bits;
	/*
	LOGI( "draw width:%d\n", buffer.width );
	LOGI( "draw height:%d\n", buffer.height );
	LOGI( "draw stride:%d\n", buffer.stride );
	*/
	for( int32_t y = 0; y < buffer.height; y++ ) {
		unsigned char* lines = bits;
		for( int32_t x = 0; x < buffer.width; x++ ) {
			lines[0] = 0xff;
			lines[1] = 0xff;
			lines[2] = 0;
			lines[3] = 0xff;
			lines += 4;
		}
		bits += buffer.stride*sizeof(int32_t);
	}
	ANativeWindow_unlockAndPost( engine->app->window  );
}

/**
 */
static void engine_term_display(struct engine* engine) {
    engine->animating = 0;
}

/**
 * Process the next input event.
 * http://wdnet.jp/library/android/androidapi#2.4.3
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	int32_t type = AInputEvent_getType(event);
	if( type == AINPUT_EVENT_TYPE_MOTION ) {
		int32_t src = AInputEvent_getSource(event);	// 入力デバイスの種類
		// src == AINPUT_SOURCE_TOUCHSCREEN タッチスクリーン
		// src == AINPUT_SOURCE_MOUSE AINPUT_SOURCE_TRACKBALL AINPUT_SOURCE_TOUCHPAD
		int32_t action = AMotionEvent_getAction(event);
		int32_t meta = AMotionEvent_getMetaState(event);
		// AMotionEvent_getEventTime(event); // イベント発生時間
		// AMotionEvent_getDownTime(event); // 押されていた時間
		// AMotionEvent_getEdgeFlags(event); // スクリーン端判定
		float x = AMotionEvent_getX(event, 0);
		float y = AMotionEvent_getY(event, 0);
		float cy = AMotionEvent_getTouchMajor(event,0);	// 触れられている長辺 指の形状から縦側にしておく
		float cx = AMotionEvent_getTouchMinor(event,0);	// 触れられている短辺 指の形状から横側にしておく
		float pressure = AMotionEvent_getPressure(event, 0);	// 圧力
		/*
		float size = AMotionEvent_getSize(event, 0);	// 範囲(推定値) デバイス固有値から0-1の範囲に正規化したもの
		float toolmajor = AMotionEvent_getToolMajor(event,0);
		float toolminor = AMotionEvent_getToolMinor(event,0);
		LOGI( "press : %f, size: %f, major : %f, minor : %f\n", pressure, size, toolmajor, toolminor );
		*/
		int32_t id = AMotionEvent_getPointerId(event, 0);
		action &= AMOTION_EVENT_ACTION_MASK;
		switch( action ) {
		case AMOTION_EVENT_ACTION_DOWN:
			engine->OnTouchDown( x, y, cx, cy, id, pressure, meta );
			break;
		case AMOTION_EVENT_ACTION_UP:
			engine->OnTouchUp( x, y, cx, cy, id, pressure, meta );
			break;
		case AMOTION_EVENT_ACTION_CANCEL:	// Down/Up同時発生。ありえるの？
			break;
		case AMOTION_EVENT_ACTION_MOVE:
			engine->OnTouchMove( x, y, cx, cy, id, pressure, meta );
			break;
		case AMOTION_EVENT_ACTION_POINTER_DOWN: {	// multi-touch
			size_t downidx = (action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
			size_t count = AMotionEvent_getPointerCount(event);
			if( downidx == 0 ) {
				engine->OnTouchDown( x, y, cx, cy, id, pressure, meta );
			} else {
				engine->OnTouchMove( x, y, cx, cy, id, pressure, meta );
			}
			for( size_t i = 1; i < count; i++ ) {
				x = AMotionEvent_getX(event, i);
				y = AMotionEvent_getY(event, i);
				cy = AMotionEvent_getTouchMajor(event,i);
				cx = AMotionEvent_getTouchMinor(event,i);
				pressure = AMotionEvent_getPressure(event, i);
				id = AMotionEvent_getPointerId(event, i);
				if( i == downidx ) {
					engine->OnTouchDown( x, y, cx, cy, id, pressure, meta );
				} else {
					engine->OnTouchMove( x, y, cx, cy, id, pressure, meta );
				}
			}
			break;
		}
		case AMOTION_EVENT_ACTION_POINTER_UP: {	// multi-touch
			size_t upidx = (action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
			size_t count = AMotionEvent_getPointerCount(event);
			if( upidx == 0 ) {
				engine->OnTouchUp( x, y, cx, cy, id, pressure, meta );
			} else {
				engine->OnTouchMove( x, y, cx, cy, id, pressure, meta );
			}
			for( size_t i = 1; i < count; i++ ) {
				x = AMotionEvent_getX(event, i);
				y = AMotionEvent_getY(event, i);
				cy = AMotionEvent_getTouchMajor(event,i);
				cx = AMotionEvent_getTouchMinor(event,i);
				pressure = AMotionEvent_getPressure(event, i);
				id = AMotionEvent_getPointerId(event, i);
				if( i == upidx ) {
					engine->OnTouchUp( x, y, cx, cy, id, pressure, meta );
				} else {
					engine->OnTouchMove( x, y, cx, cy, id, pressure, meta );
				}
			}
			break;
		}
		case AMOTION_EVENT_ACTION_OUTSIDE:
			break;
		}
		engine->animating = 1;
		return 1;
	} else if( type == AINPUT_EVENT_TYPE_KEY ) { // key events
		int32_t src = AInputEvent_getSource(event);	// 入力デバイスの種類
		// src == AINPUT_SOURCE_KEYBOARD AINPUT_SOURCE_DPAD
		return 1;
	}
	return 0;
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(struct saved_state));
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                        engine->accelerometerSensor, (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                        engine->accelerometerSensor);
            }
            // Also stop animating.
            engine->animating = 0;
            engine_draw_frame(engine);
            break;
    }
}


/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    struct engine engine;

    // Make sure glue isn't stripped.
    app_dummy();

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstance();
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager,
            ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager,
            state->looper, LOOPER_ID_USER, NULL, NULL);

    if (state->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(struct saved_state*)state->savedState;
    }

    // loop waiting for stuff to do.

    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
                (void**)&source)) >= 0) {

            // Process this event.
            if (source != NULL) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                            &event, 1) > 0) {
                        // LOGI("accelerometer: x=%f y=%f z=%f", event.acceleration.x, event.acceleration.y, event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                engine_term_display(&engine);
                return;
            }
        }

        if (engine.animating) {
            // Done with events; draw next animation frame.
            engine.state.angle += .01f;
            if (engine.state.angle > 1) {
                engine.state.angle = 0;
            }

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            engine_draw_frame(&engine);
        }
    }
}
