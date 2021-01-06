#ifndef _PFC_PROFILER_H_
#define _PFC_PROFILER_H_

#ifdef _WIN32
#define STRICT
#include <windows.h>
#endif

class profiler_static
{
private:
	const char * name;
  int64_t total_time,num_called;

public:
	profiler_static(const char * p_name)
	{
		name = p_name;
		total_time = 0;
		num_called = 0;
	}
	~profiler_static()
	{
		char blah[256];
		char total_time_text[128];
		char num_text[128];
#ifdef AVS_WINDOWS
		_i64toa(total_time,total_time_text,10);
		_i64toa(num_called,num_text,10);
#else
    sprintf(total_time_text, "%ld", total_time);
    sprintf(num_text, "%ld", num_called);
#endif
sprintf(blah,"profiler: %s - %s cycles (executed %s times)\n",name,total_time_text,num_text);
#ifdef _WIN32
		OutputDebugStringA(blah);
#endif
	}
	void add_time(int64_t delta) {total_time+=delta;num_called++;}
};

class profiler_local
{
private:
	static int64_t get_timestamp();
  int64_t start;
	profiler_static * owner;
public:
	profiler_local(profiler_static * p_owner)
	{
		owner = p_owner;
		start = get_timestamp();
	}
	~profiler_local()
	{
    int64_t end = get_timestamp();
		owner->add_time(end-start);
	}

};

#define profiler(name) \
	static profiler_static profiler_static_##name(#name); \
	profiler_local profiler_local_##name(&profiler_static_##name);




#endif