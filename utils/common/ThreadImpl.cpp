//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Thread base class
//---------------------------------------------------------------------------
#define NOMINMAX
#include "tjsCommHead.h"

#include <process.h>
#include <algorithm>

#include "ThreadIntf.h"
#include "ThreadImpl.h"
#include "MsgIntf.h"



//---------------------------------------------------------------------------
// tTVPThread : a wrapper class for thread
//---------------------------------------------------------------------------
tTVPThread::tTVPThread(bool suspended) : Terminated(false), Suspended(suspended), runnable_(this)
{
	if( suspended ) {
	} else {
		thread_.start(runnable_);
	}
}
//---------------------------------------------------------------------------
tTVPThread::~tTVPThread()
{
}
//---------------------------------------------------------------------------
void tTVPThread::WaitFor()
{
	thread_.join();
}
//---------------------------------------------------------------------------
tTVPThreadPriority tTVPThread::GetPriority()
{
	POCO::Thread::Priority n = thread_.getPriority();
	switch(n)
	{
	case POCO::Thread::PRIO_LOWEST:	return ttpLowest;
	case POCO::Thread::PRIO_LOW:	return ttpLower;
	case POCO::Thread::PRIO_NORMAL:	return ttpNormal;
	case POCO::Thread::PRIO_HIGH:	return ttpHigher;
	case POCO::Thread::PRIO_HIGHEST:return ttpHighest;
	}

	return ttpNormal;
}
//---------------------------------------------------------------------------
void tTVPThread::SetPriority(tTVPThreadPriority pri)
{
	POCO::Thread::Priority npri = POCO::Thread::PRIO_NORMAL:
	switch(pri)
	{
	case ttpIdle:			npri = POCO::Thread::PRIO_LOWEST;	break;
	case ttpLowest:			npri = POCO::Thread::PRIO_LOWEST;	break;
	case ttpLower:			npri = POCO::Thread::PRIO_LOW;		break;
	case ttpNormal:			npri = POCO::Thread::PRIO_NORMAL;	break;
	case ttpHigher:			npri = POCO::Thread::PRIO_HIGH;		break;
	case ttpHighest:		npri = POCO::Thread::PRIO_HIGHEST;	break;
	case ttpTimeCritical:	npri = POCO::Thread::PRIO_HIGHEST;	break;
	}

	thread_.setPriority(npri);
}
//---------------------------------------------------------------------------
void tTVPThread::Suspend()
{
	thread_.trySleep(0x7ffffffe);
}
//---------------------------------------------------------------------------
void tTVPThread::Resume()
{
	if( Suspended ) {
		Suspended = false;
		thread_.start(runnable_);
	} else {
		thread_.wakeUp();
	}
}
//---------------------------------------------------------------------------


#ifdef _WIN32
//---------------------------------------------------------------------------
tjs_int TVPDrawThreadNum = 1;

struct ThreadInfo {
  bool readyToExit;
  HANDLE thread;
  TVP_THREAD_TASK_FUNC  lpStartAddress;
  TVP_THREAD_PARAM lpParameter;
};
static std::vector<ThreadInfo*> TVPThreadList;
static std::vector<tjs_int> TVPProcesserIdList;
static LONG TVPRunningThreadCount = 0;
static tjs_int TVPThreadTaskNum, TVPThreadTaskCount;

//---------------------------------------------------------------------------
static tjs_int GetProcesserNum(void)
{
  static tjs_int processor_num = 0;
  if (! processor_num) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    processor_num = info.dwNumberOfProcessors;
  }
  return processor_num;
}

tjs_int TVPGetProcessorNum(void)
{
  return GetProcesserNum();
}

//---------------------------------------------------------------------------
tjs_int TVPGetThreadNum(void)
{
  tjs_int threadNum = TVPDrawThreadNum ? TVPDrawThreadNum : GetProcesserNum();
  threadNum = std::min(threadNum, TVPMaxThreadNum);
  return threadNum;
}

//---------------------------------------------------------------------------
static DWORD WINAPI ThreadLoop(LPVOID p)
{
  ThreadInfo *threadInfo = (ThreadInfo*)p;
  for(;;) {
    if (threadInfo->readyToExit)
      break;
    (threadInfo->lpStartAddress)(threadInfo->lpParameter);
    InterlockedDecrement(&TVPRunningThreadCount);
    SuspendThread(GetCurrentThread());
  }
  delete threadInfo;
  ExitThread(0);

  return TRUE;
}
//---------------------------------------------------------------------------
void TVPBeginThreadTask(tjs_int taskNum)
{
  TVPThreadTaskNum = taskNum;
  TVPThreadTaskCount = 0;
  tjs_int extraThreadNum = TVPGetThreadNum() - 1;
  if (TVPProcesserIdList.empty()) {
    DWORD processAffinityMask, systemAffinityMask;
    GetProcessAffinityMask(GetCurrentProcess(),
                           &processAffinityMask,
                           &systemAffinityMask);
    for (tjs_int i = 0; i < MAXIMUM_PROCESSORS; i++) {
      if (processAffinityMask & (1 << i))
        TVPProcesserIdList.push_back(i);
    }
    if (TVPProcesserIdList.empty())
      TVPProcesserIdList.push_back(MAXIMUM_PROCESSORS);
  }
  while ( static_cast<tjs_int>(TVPThreadList.size()) < extraThreadNum) {
    ThreadInfo *threadInfo = new ThreadInfo();
    threadInfo->readyToExit = false;
    threadInfo->thread = CreateThread(NULL, 0, ThreadLoop, threadInfo, CREATE_SUSPENDED, NULL);
    SetThreadIdealProcessor(threadInfo->thread, TVPProcesserIdList[TVPThreadList.size() % TVPProcesserIdList.size()]);
    TVPThreadList.push_back(threadInfo);
  }
  while ( static_cast<tjs_int>(TVPThreadList.size()) > extraThreadNum) {
    ThreadInfo *threadInfo = TVPThreadList.back();
    threadInfo->readyToExit = true;
    while (ResumeThread(threadInfo->thread) == 0)
      Sleep(0);
    TVPThreadList.pop_back();
  }
}

//---------------------------------------------------------------------------
void TVPExecThreadTask(TVP_THREAD_TASK_FUNC func, TVP_THREAD_PARAM param)
{
  if (TVPThreadTaskCount >= TVPThreadTaskNum - 1) {
    func(param);
    return;
  }    
  ThreadInfo *threadInfo;
  threadInfo = TVPThreadList[TVPThreadTaskCount++];
  threadInfo->lpStartAddress = func;
  threadInfo->lpParameter = param;
  InterlockedIncrement(&TVPRunningThreadCount);
  while (ResumeThread(threadInfo->thread) == 0)
    Sleep(0);
}
//---------------------------------------------------------------------------
void TVPEndThreadTask(void) 
{
  while ((LONG)InterlockedCompareExchange(&TVPRunningThreadCount, 0, 0) != 0)
    Sleep(0);
}

//---------------------------------------------------------------------------
#else
// POCO ThreadPool を使った実装に
tjs_int TVPDrawThreadNum = 1;
static Poco::ThreadPool* TVPThreadPool = NULL;
static Poco::NotificationQueue* TVPNotificationQueue = NULL;
static std::vector<tTVPTaskNotification*> TVPtTaskNotifications;
//---------------------------------------------------------------------------
tjs_int TVPGetProcessorNum(void)
{
  return POCO::Environment::processorCount();
}
//---------------------------------------------------------------------------
tjs_int TVPGetThreadNum(void)
{
  tjs_int threadNum = TVPDrawThreadNum ? TVPDrawThreadNum : TVPGetProcessorNum();
  threadNum = std::min(threadNum, TVPMaxThreadNum);
  return threadNum;
}
//---------------------------------------------------------------------------
void TVPBeginThreadTask(tjs_int taskNum) {
	if( TVPThreadPool == NULL ) {
		TVPThreadPool = new Poco::ThreadPool();
		tjs_int count = TVPGetThreadNum();
		for( tjs_int i = 0; i < count; i++ ) {
			TVPThreadPool->start( new tTVPWorker(i,) );
		}
	}
}
//---------------------------------------------------------------------------
void TVPExecThreadTask(TVP_THREAD_TASK_FUNC func, TVP_THREAD_PARAM param) {
	if( TVPThreadPool == NULL ) return;
	if( TVPNotificationQueue ) {
		TVPNotificationQueue->enqueueNotification( new WorkNotification( func, param ) );
		// WorkNotification もプールした方が良さそうな
	}
}
//---------------------------------------------------------------------------
void TVPEndThreadTask(void) {
	if( TVPThreadPool == NULL ) return;
	// キューが空になるまで待機… って、空になってもまだ処理中の可能性があるか TODO
	while( !TVPThreadPool->empty() ) Poco::Thread::sleep(1);
}
//---------------------------------------------------------------------------
void TVPExitWorkerThread(void) {
	if( TVPNotificationQueue ) {
		TVPNotificationQueue->wakeUpAll();
	}
	if( TVPThreadPool ) {
		TVPThreadPool->joinAll();
	}
}
//---------------------------------------------------------------------------
// ワーカースレッドへの通知クラス
class tTVPTaskNotification : public Poco::Notification {
	TVP_THREAD_TASK_FUNC func_;
	TVP_THREAD_PARAM param_;

public:
	tTVPTaskNotification( TVP_THREAD_TASK_FUNC func, TVP_THREAD_PARAM param): func_(func), param_(param) {}

	void setTask( TVP_THREAD_TASK_FUNC func, TVP_THREAD_PARAM param) {
		func_ = func;
		param_ = param;
	}

	void execute() {
		func_( param_ );
	}
};

// ワーカースレッドクラス
// 通知キューから通知オブジェクトを取り出し、実行する。
class tTVPWorker: public Poco::Runnable {
	int index_;
	Poco::NotificationQueue&	queue_;

public:
	tTVPWorker(int name, Poco::NotificationQueue& queue):index_(name), queue_(queue) {}

	// スレッドのメイン処理
	void run() {
		while(true) {
			// 通知キューからデータが取得できるまで待機
			Poco::AutoPtr<Poco::Notification> notify(queue_.waitDequeueNotification());
			if( notify ) {
				tTVPTaskNotification* task = dynamic_cast<tTVPTaskNotification*>(notify.get());
				if( task ) {
					task->execute();
				}
			} else {
				break;  // 返値がNULLのときはスレッドを終了
			}
		}
	}
};




#endif
