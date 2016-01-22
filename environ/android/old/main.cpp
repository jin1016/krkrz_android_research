
#if 0
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
#endif
/**
 * Process the next input event.
 * http://wdnet.jp/library/android/androidapi#2.4.3
 */


	// I/native-activity(1396): internal path:/data/data/jp.kirikiri.krkrz/files
	// I/native-activity(1396): external path:/storage/emulated/0/Android/data/jp.kirikiri.krkrz/files

// include/android/native_activity.h
/*
class Activity {
	ANativeActivity* aciviity_;
public:
	void set( ANativeActivity* a ) { aciviity_ = a; }

	int32_t getSdkVersion() const { return aciviity_.sdkVersion }
	
	aciviity_->assetManager
}
class Application {
	struct android_app* app_;

	ANativeActivity* getAcitivity() { return app_->activity; }
	const ANativeActivity* getAcitivity() const { return app_->activity; }
};

	// I/native-activity(1396): internal path:/data/data/jp.kirikiri.krkrz/files
	// I/native-activity(1396): external path:/storage/emulated/0/Android/data/jp.kirikiri.krkrz/files
	LOGI( "internal path:%s\n", state->activity->internalDataPath );
	LOGI( "external path:%s\n", state->activity->externalDataPath );
    // loop waiting for stuff to do.

*/

/*
class MessageHandler {
	int pipe_[2];
	class NativeEvent : android_poll_source {
	};
	struct android_poll_source data_;

	static void handler(struct android_app* app, struct android_poll_source* source);

	void create( ALooper* looper ) {
		int ret = pipe2( pipe_, O_NONBLOCK | O_CLOEXEC );
		assert( ret != -1 );

		data_.id = LOOPER_ID_INPUT;
		data_.app = NULL;
		data_.process = handler;
	
		ALooper_addFd( looper, pipe_[0], LOOPER_ID_USER, ALOOPER_EVENT_INPUT, NULL, &data_ );
	}
};


*/


