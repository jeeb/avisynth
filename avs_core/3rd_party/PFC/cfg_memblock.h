#ifndef _PFC_CFG_VAR_MEMBLOCK_H_
#define _PFC_CFG_VAR_MEMBLOCK_H_

template<class T>
class cfg_memblock : public cfg_var, public mem_block_list<T>
{
private:
	const T* init_val;
	int init_size;
protected:
	virtual void reset()
	{
		remove_all();
		if (init_val)
		{
			int n;
			for(n=0;n<init_size;n++)
			{
				add_item(init_val[n]);
			}
		}
	}

	virtual void get_raw_data(write_config_callback * out)
	{
		out->write(get_ptr(),get_count()*sizeof(T));
	}

	virtual void set_raw_data(const void * data,int size)
	{
		remove_all();
		T * ptr = (T*)data;
		int count = size/sizeof(T);
		int n;
		for(n=0;n<count;n++)
		{
			add_item(ptr[n]);
		}
	}

public:
	cfg_memblock(const char * name) : cfg_var(name) {init_val=0;init_size=0;}
	cfg_memblock(const char * name,const T* p_init_val,int p_init_size) : cfg_var(name)
	{
		init_val = p_init_val;
		init_size = p_init_size;
		int n;
		for(n=0;n<init_size;n++)
		{
			add_item(init_val[n]);
		}
	}
};

#endif