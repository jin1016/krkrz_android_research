
#ifndef __T_APPLICATION_H__
#define __T_APPLICATION_H__

#include <vector>
#include <map>
#include <stack>

enum {
	mrOk,
	mrAbort,
	mrCancel,
};

enum {
  mtCustom = 0,
  mtWarning,
  mtError,
  mtInformation,
  mtConfirmation,
  mtStop,
};

enum {
	mbOK = 1,
};

class tTVPApplication {
	std::vector<class TTVPWindowForm*> windows_list_;
	std::wstring title_;

	bool is_attach_console_;
	std::wstring console_title_;

	bool tarminate_;
	bool application_activating_;
	bool has_map_report_process_;

	class tTVPAsyncImageLoader* image_load_thread_;

	std::stack<class tTVPWindow*> modal_window_stack_;

	std::wstring internal_data_path_;
	std::wstring external_data_path_;

private:
	void CheckConsole();
	void CloseConsole();
	void CheckDigitizer();
	void ShowException( const wchar_t* e );
	void Initialize() {}
	void Run();

public:
	tTVPApplication();
	~tTVPApplication();
	bool StartApplication( int argc, char* argv[] );

	void PrintConsole( const wchar_t* mes, unsigned long len, bool iserror = false );
	bool IsAttachConsole() { return is_attach_console_; }

	bool IsTarminate() const { return tarminate_; }

	// HWND GetHandle();

	bool IsIconic() {
		/*
		HWND hWnd = GetHandle();
		if( hWnd != INVALID_HANDLE_VALUE ) {
			return 0 != ::IsIconic(hWnd);
		}
		return true; // そもそもウィンドウがない
		*/
		return false;
	}
	void Minimize();
	void Restore();
	void BringToFront();

	void AddWindow( class TTVPWindowForm* win ) {
		windows_list_.push_back( win );
	}
	void RemoveWindow( class TTVPWindowForm* win );
	unsigned int GetWindowCount() const {
		return (unsigned int)windows_list_.size();
	}

	/*
	void FreeDirectInputDeviceForWindows();

	bool ProcessMessage( MSG &msg );
	void ProcessMessages();
	void HandleMessage();
	void HandleIdle(MSG &msg);
	*/

	std::wstring GetTitle() const { return title_; }
	void SetTitle( const std::wstring& caption );

	static inline int MessageDlg( const std::wstring& string, const std::wstring& caption, int type, int button ) {
		//return ::MessageBox( NULL, string.c_str(), caption.c_str(), type|button  );
		return 0;
	}
	void Terminate() {
		// ::PostQuitMessage(0);
		exit(0);
	}

	// HWND GetMainWindowHandle() const;

	int ArgC;
	char ** ArgV;

	//void PostMessageToMainWindow(UINT message, WPARAM wParam, LPARAM lParam);


	/*
	void ModalStarted( class tTVPWindow* form ) {
		modal_window_stack_.push(form);
	}
	void ModalFinished();
	*/
	void OnActiveAnyWindow();
	void DisableWindows();
	void EnableWindows( const std::vector<class TTVPWindowForm*>& win );
	void GetDisableWindowList( std::vector<class TTVPWindowForm*>& win );
	void GetEnableWindowList( std::vector<class TTVPWindowForm*>& win, class TTVPWindowForm* activeWindow );

	/*
	void RegisterAcceleratorKey(HWND hWnd, char virt, short key, short cmd);
	void UnregisterAcceleratorKey(HWND hWnd, short cmd);
	void DeleteAcceleratorKeyTable( HWND hWnd );

	void OnActivate( HWND hWnd );
	void OnDeactivate( HWND hWnd );
	*/
	bool GetActivating() const { return application_activating_; }
	bool GetNotMinimizing() const;

	const std::wstring& GetInternalDataPath() const { return internal_data_path_; }
	const std::wstring& GetExternalDataPath() const { return external_data_path_; }
	/**
	 * 画像の非同期読込み要求
	 */
	void LoadImageRequest( class iTJSDispatch2 *owner, class tTJSNI_Bitmap* bmp, const ttstr &name );
};
std::vector<std::string>* LoadLinesFromFile( const std::wstring& path );

//inline HINSTANCE GetHInstance() { return ((HINSTANCE)GetModuleHandle(0)); }
extern class tTVPApplication* Application;


#endif // __T_APPLICATION_H__
