//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Thread base class
//---------------------------------------------------------------------------
#ifndef ThreadImplH
#define ThreadImplH
#include "tjsNative.h"
#include "ThreadIntf.h"

#include "Poco/Thread.h"
#include "Poco/Runnable.h"
#include "Poco/Event.h"

//---------------------------------------------------------------------------
// tTVPThread
//---------------------------------------------------------------------------
class tTVPThread
{
	bool Terminated;
	bool Suspended;

	Poco::Thread	thread_;

	class tTVPThreadRunnable : public Poco::Runnable {
		tTVPThread* owner_;
	public:
		tTVPThreadRunnable( tTVPThread* owner ) : owner_(owner) {}
		void run() {
			owner_->Execute();
		}
	};
	tTVPThreadRunnable runnable_;
	friend class tTVPThreadRunnable;

public:
	tTVPThread(bool suspended);
	virtual ~tTVPThread();

	bool GetTerminated() const { return Terminated; }
	void SetTerminated(bool s) { Terminated = s; }
	void Terminate() { Terminated = true; }

protected:
	virtual void Execute() = 0;

public:
	void WaitFor();

	tTVPThreadPriority GetPriority();
	void SetPriority(tTVPThreadPriority pri);

	void Suspend();
	void Resume();
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTVPThreadEvent
// 完全にPOCO任せ、Windowsでは従来と同じ実装(ただし一部例外が発生する可能性あり)
//---------------------------------------------------------------------------
class tTVPThreadEvent
{
	Poco::Event event_;

public:
	tTVPThreadEvent(bool manualreset = false) : event_(!manualreset) {}
	virtual ~tTVPThreadEvent() {}

	void Set() { event_.set(); }
	void Reset() { event_.reset(); }
	bool WaitFor(tjs_uint timeout) {
		if( timeout == 0 ) {
			event_.wait();
			return true;
		} else {
			return event_.tryWait(timeout);
		}
	}
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif
