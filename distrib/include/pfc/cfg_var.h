//deprecated
#ifndef _PFC_CFG_VAR_H_
#define _PFC_CFG_VAR_H_

class cfg_var_notify;

//IMPORTANT:
//cfg_var objects are intended ONLY to be created statically !!!!


class NOVTABLE cfg_var
{
private:
	string_simple var_name;
	static cfg_var * list;
	cfg_var * next;
	cfg_var_notify * notify_list;

public:
	class NOVTABLE write_config_callback
	{
	public:
		virtual void write(const void * ptr,int bytes)=0;
	};

	class write_config_callback_i : public write_config_callback
	{
	public:
		mem_block data;
		write_config_callback_i() {data.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);}
		virtual void write(const void * ptr,int bytes) {data.append(ptr,bytes);}
	};

protected:
	cfg_var(const char * name) : var_name(name) {next=list;list=this;notify_list=0;};
	
	const char * var_get_name() const {return var_name;}

	//override me
	virtual void get_raw_data(write_config_callback * out)=0;
	virtual void set_raw_data(const void * data,int size)=0;

	void on_change();//call this whenever your contents change

public:

	const char * get_name() const {return var_name;}

	virtual void reset()=0;//override me
	void refresh() {on_change();}
	void add_notify(cfg_var_notify *);
	static void config_read_file(const void * src,int size);
	static void config_write_file(write_config_callback * out);
	static void config_reset();
	static void config_on_app_init();//call this first from main()
};

class cfg_int : public cfg_var
{
private:
	int val,def;
	virtual void get_raw_data(write_config_callback * out) {out->write(&val,sizeof(val));}
	virtual void set_raw_data(const void * data,int size) {if (size==sizeof(int)) {val=*(int*)data;on_change();}}
public:
	virtual void reset() {val=def;on_change();}
	explicit inline cfg_int(const char * name,int v) : cfg_var(name) {val=def=v;}
	inline int operator=(const cfg_int & v) {val=v;on_change();return val;}
	inline operator int() const {return val;}
	inline int operator=(int v) {val=v;on_change();return val;}
	inline int get_def() {return def;}

};

class cfg_string : public cfg_var
{
private:
	string_simple val,def;
	
	virtual void get_raw_data(write_config_callback * out)
	{
		out->write((const char*)val,strlen(val) * sizeof(char));
	}

	virtual void set_raw_data(const void * data,int size)
	{
		val.set_string_n((const char*)data,size/sizeof(char));
		on_change();
	}

public:
	virtual void reset() {val=def;on_change();}
	explicit inline cfg_string(const char * name,const char * v) : cfg_var(name), val(v), def(v) {}
	void operator=(const cfg_string & v) {val=v.get_val();on_change();}
	inline operator const char * () const {return val;}
	inline const char * operator=(const char* v) {val=v;on_change();return val;}
	inline const char * get_val() const {return val;}
	inline bool is_empty() {return !val[0];}
};

#if 0
class cfg_string_mt : public cfg_var//multithread-safe version
{
private:
	critical_section sync;
	string8 val,def;
	
	virtual void get_raw_data(write_config_callback * out)
	{
		insync(sync);
		out->write(val.get_ptr(),val.length() * sizeof(char));
	}

	virtual void set_raw_data(const void * data,int size)
	{
		insync(sync);
		val.reset();
		val.add_string_n((const char*)data,size/sizeof(char));
		on_change();
	}

public:
	virtual void reset() {insync(sync);val=def;on_change();}
	cfg_string_mt(const char * name,const char * v) : cfg_var(name), val(v), def(v) {}
	void get_val(string8 & out) {insync(sync);out=val;}
	void set_val(const char * v) {insync(sync);val=v;on_change();}
};
#endif

template<class T>
class cfg_struct_t : public cfg_var
{
private:
	T val,def;


	virtual void get_raw_data(write_config_callback * out)
	{
		out->write(&val,sizeof(val));
	}

	virtual void set_raw_data(const void * data,int size)
	{
		if (size==sizeof(T))
		{
			memcpy(&val,data,sizeof(T));
			on_change();
		}
	}
public:
	inline virtual void reset() {val=def;on_change();}
	explicit inline cfg_struct_t(const char * name,T v) : cfg_var(name) {val=def=v;}
	explicit inline cfg_struct_t(const char * name,int filler) : cfg_var(name) {memset(&val,filler,sizeof(T));memset(&def,filler,sizeof(T));}
	inline const T& operator=(const cfg_struct_t<T> & v) {val = v.get_val();on_change();return val;}
	inline T& get_val() {return val;}
	inline const T& get_val() const {return val;}
	inline operator T() const {return val;}
	inline const T& operator=(const T& v) {val=v;on_change();return val;}
};

class cfg_var_notify
{
	friend cfg_var;
private:
	cfg_var * my_var;
	cfg_var_notify * next;
	cfg_var_notify * var_next;
	static cfg_var_notify * list;
public:
	cfg_var_notify(cfg_var * var) {my_var=var;next=list;list=this;var_next=0;}
	//override me
	virtual void on_var_change(const char * name,cfg_var * var) {}

	static void on_app_init();//called by cfg_var::config_on_app_init()
};

class cfg_var_notify_func : public cfg_var_notify
{
private:
	void (*func)();
public:
	cfg_var_notify_func(cfg_var * var,void (*p_func)() ) : cfg_var_notify(var), func(p_func) {}
	virtual void on_var_change(const char * name,cfg_var * var) {func();}
};


#endif
