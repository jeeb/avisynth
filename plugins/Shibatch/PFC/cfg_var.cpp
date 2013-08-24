#include "pfc.h"


cfg_var * cfg_var::list=0;

void cfg_var::config_reset()
{
	cfg_var * ptr;
	for(ptr = list; ptr; ptr=ptr->next) ptr->reset();
}



void cfg_var::config_read_file(const void * _src,int size)
{
	mem_block temp;
	const BYTE * src = (BYTE*)_src;
	int src_ptr;
	for(src_ptr=0;src_ptr<size;)
	{
		if (size - src_ptr<4) break;
		long t_size = *(long*)(src+src_ptr);
		if (t_size<=0) return;
		src_ptr+=4;
		if (size - src_ptr < t_size) break;
		string8 name;
		name.add_string_n((char*)(src+src_ptr),t_size);
		src_ptr+=t_size;

		if (size - src_ptr<4) break;
		t_size = *(long*)(src+src_ptr);
		if (t_size<0) return;
		src_ptr+=4;
		if (size - src_ptr < t_size) break;


		cfg_var * ptr;
		for(ptr = list; ptr; ptr=ptr->next)
		{
			if (!strcmp(ptr->var_name,name))
			{
				ptr->set_raw_data(src + src_ptr,t_size);
				break;
			}
		}
		src_ptr+=t_size;
	}
}

void cfg_var::config_write_file(write_config_callback * out)
{
	cfg_var * ptr;
	write_config_callback_i temp;
	for(ptr = list; ptr; ptr=ptr->next)
	{
		temp.data.set_size(0);
		long size = strlen(ptr->var_name);
		out->write(&size,4);
		out->write((const char*)ptr->var_name,size);
		ptr->get_raw_data(&temp);
		size = temp.data.get_size();
		out->write(&size,4);
		if (size>0) out->write(temp.data.get_ptr(),size);
	}
}

void cfg_var::config_on_app_init()
{
	cfg_var_notify::on_app_init();
}

void cfg_var_notify::on_app_init()
{
	cfg_var_notify * ptr = list;
	while(ptr)
	{
		if (ptr->my_var)
			ptr->my_var->add_notify(ptr);
		ptr=ptr->next;
	}
}

void cfg_var::on_change()
{
	cfg_var_notify * ptr = notify_list;
	while(ptr)
	{
		ptr->on_var_change(var_get_name(),this);
		ptr = ptr->var_next;
	}
	
}

cfg_var_notify * cfg_var_notify::list=0;

void cfg_var::add_notify(cfg_var_notify * ptr)
{
	ptr->var_next = notify_list;
	notify_list = ptr;
}
