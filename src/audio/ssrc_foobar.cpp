#include "../../SDK/foobar2000.h"
#include "ssrc.h"
#include "../resource.h"


static cfg_int cfg_srate("resampler_srate",48000),cfg_slow("resampler_slow",0);

int CanResample(int sfrq,int dfrq);

class dsp_ssrc : public dsp_i_base_t<resampler>
{
	Resampler_base * res;
	UINT nch,srate,dest_srate;
	bool fast;
	bool whined;

	int srate_override;
	int slow_override;
	bool have_slow_override;

	bool setup_chunk(audio_chunk* chunk)
	{
		int size;
		audio_sample * ptr = res->GetBuffer(&size);
		if (size>0)
		{
			if (chunk==0) chunk = insert_chunk(size);
			chunk->set_data(ptr,size/nch,nch,dest_srate);
			res->Read(size);
			return true;
		}
		else return false;
	}

	virtual void set_dest_sample_rate(unsigned p_srate)
	{
		srate_override = p_srate;
	}

	virtual void set_config(unsigned flags)
	{
		slow_override = (flags & resampler::FLAG_SLOW) ? 1 : 0;
		have_slow_override = true;
	}

	virtual bool is_conversion_supported(unsigned src_srate,unsigned dst_srate)
	{
		return !!CanResample(src_srate,dst_srate);
	}

public:
	virtual GUID get_guid()
	{
		// {8EFCFE86-EA25-4933-A621-529FCE49046B}
		static const GUID guid = 
		{ 0x8efcfe86, 0xea25, 0x4933, { 0xa6, 0x21, 0x52, 0x9f, 0xce, 0x49, 0x4, 0x6b } };
		return guid;
	}

	virtual const char * get_name() {return "Resampler (SSRC)";}

	virtual bool on_chunk(audio_chunk * chunk)
	{
		UINT new_dest_srate = srate_override>0 ? srate_override : cfg_srate;
		bool new_fast = have_slow_override ? !slow_override : !cfg_slow;

		if (res && (chunk->get_channels()!=nch || chunk->get_srate()!=srate || new_dest_srate!=dest_srate || fast!=new_fast))
		{
			res->Finish();
			setup_chunk(0);
			delete res;
			res=0;
		}

		nch=chunk->get_channels();
		srate=chunk->get_srate();
		dest_srate = new_dest_srate;
		fast = new_fast;

		if (!res)
		{
			if (dest_srate==srate) return true;//same specs, dont resample

			res = SSRC_create(srate,dest_srate,nch,2,1,fast);
			if (!res)
			{
				if (!whined)
				{
					console::error(uStringPrintf("unable to resample from %uHz to %uHz",srate,dest_srate));
					whined = true;
				}
				return true;//FUCKO: unsupported src/dest samplerate combo, could make alt cheap linear resampler for those
			}
		}

		res->Write(chunk->get_data(),chunk->get_data_length());

		return setup_chunk(chunk);
	}


	virtual void on_endofplayback()
	{
		if (res)
		{
			res->Finish();
			setup_chunk(0);
			delete res;
			res=0;
		}
	}

	virtual void flush()
	{
		if (res) {delete res;res=0;}
	}
	
	virtual double get_latency()
	{//todo
		return res ? (double)res->GetLatency()/1000.0 : 0;
	}

	dsp_ssrc()
	{
		have_slow_override = false;
		srate_override = 0;
		slow_override = 0;
		res=0;
		srate=0;
		nch=0;
		fast=0;
		whined = false;
	}

	~dsp_ssrc()
	{
		if (res) delete res;
	}
};

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(wnd,IDC_SAMPLERATE);
			int n;
			for(n=0;n<tabsize(srate_tab);n++)
			{
				char temp[16];
				wsprintfA(temp,"%u",srate_tab[n]);
				SendMessageA(list,CB_ADDSTRING,0,(long)temp);
			}			
			SetDlgItemInt(wnd,IDC_SAMPLERATE,cfg_srate,0);
			SendDlgItemMessage(wnd,IDC_SLOW,BM_SETCHECK,cfg_slow,0);
		}
		break;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_SLOW:
			cfg_slow = SendMessage((HWND)lp,BM_GETCHECK,0,0);
			break;
		case (CBN_SELCHANGE<<16)|IDC_SAMPLERATE:
			cfg_srate = srate_tab[SendMessage((HWND)lp,CB_GETCURSEL,0,0)];
			break;
		case (CBN_EDITCHANGE<<16)|IDC_SAMPLERATE:
			{
				int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
				if (t<6000) t=6000;
				else if (t>192000) t=192000;
				cfg_srate = t;
			}
			break;
		}
		break;
	}
	return 0;
}

class config_ssrc : public config
{
	virtual HWND create(HWND parent)
	{
		return CreateDialog(core_api::get_my_instance(),MAKEINTRESOURCE(IDD_CONFIG_SSRC),parent,ConfigProc);
	}
	virtual const char * get_name() {return "Resampler";}
	virtual const char * get_parent_name() {return "DSP Manager";}
};


static service_factory_t<dsp,dsp_ssrc> foo;
static service_factory_single_t<config,config_ssrc> foo2;