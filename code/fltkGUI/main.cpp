#include <iostream>
#include <iomanip>
#include <numeric>
using namespace std;


#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Roller.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Button.H>
#include <FL/names.h>


#include "IQSource/IQSource_File.h"
#include "IQSource/IQSource_SoapySDR.h"
#include "Decoder/Decoder.h"

#include "gui_utils.h"

using namespace std;
using namespace habdec;

typedef float TReal;
IQVector<TReal>	G_SAMPLES;
Decoder<TReal> G_DECODER;
IQSource* G_IQ_SRC_PTR = 0;
double G_FREQ = 100e6; // 434.274451e6;
double G_SAMPLING_RATE = 0;
double G_FREQ_SPAN = 0;
bool DO_CORRECTION = false;


void SetFrequency(double freq)
{
	if(G_IQ_SRC_PTR)
	{
		G_FREQ = freq;
		double freq_double = freq;
		G_IQ_SRC_PTR->setOption("frequency_double", &freq_double);
	}
}

void RunDecoder()
{
	size_t max_samples_num = 256*256; // it doesn't work for small batches and radio ...
	G_SAMPLES.resize(max_samples_num);

	while(1)
	{
		typedef std::chrono::nanoseconds TDur;

		auto _start = std::chrono::high_resolution_clock::now();
		size_t count = G_IQ_SRC_PTR->get( G_SAMPLES.data(), G_SAMPLES.size() );
		G_DECODER.pushSamples(G_SAMPLES);

		G_DECODER();
		TDur _duration = std::chrono::duration_cast<TDur>(std::chrono::high_resolution_clock::now() - _start);
		// std::cout<<"Decoder duration "<<_duration.count()<<" nS  "<<int(double(10e9)/_duration.count())<<" / second\n";

		static auto last_afc_time = std::chrono::high_resolution_clock::now();

		if( std::chrono::duration_cast< std::chrono::microseconds >
				(std::chrono::high_resolution_clock::now() - last_afc_time).count() > 1000000
			)
		{
			double freq_corr = G_DECODER.getFrequencyCorrection();
			if(DO_CORRECTION)
			{
				if( 100 < abs(freq_corr)  )
				{
					G_FREQ += freq_corr;
					G_IQ_SRC_PTR->setOption("frequency_double", &G_FREQ);
					G_DECODER.resetFrequencyCorrection(freq_corr);
					last_afc_time = std::chrono::high_resolution_clock::now();
				}
			}
		}
	}
}


int InitProcess(int baud, int a_bits, int a_stops, std::string fname)
{
	if(fname != "")
	{
		cout<<"File"<<endl;

		G_SAMPLING_RATE = 10027008 / 256;
		G_SAMPLES.samplingRate(G_SAMPLING_RATE);

		G_IQ_SRC_PTR = new IQSource_File<TReal>;
		G_IQ_SRC_PTR->setOption("sampling_rate_double", &G_SAMPLING_RATE);
		G_IQ_SRC_PTR->setOption("file_string", &fname);
		bool realtime = false;
		G_IQ_SRC_PTR->setOption("realtime_bool", &realtime);
		bool loop = true;
		G_IQ_SRC_PTR->setOption("loop_bool", &loop);

		if( !G_IQ_SRC_PTR->init() )
		{
			cout<<"IQSource_File::init failed."<<endl;
			return 1;
		}

		cout<<"IQSource_File::count: "<<G_IQ_SRC_PTR->count()<<endl;
	}
	else
	{
		cout<<"RADIO"<<endl;

		// size_t max_samples_num = 256 * 256; // it doesn't work for small batches...
		// G_SAMPLES.resize(max_samples_num);

		SoapySDR::KwargsList device_list = SoapySDR::Device::enumerate();
		if(!device_list.size())
		{
			cout<<"No SoapySDR devices found. Exit."<<endl;
			return 0;
		}
		auto& device = device_list.back();

		G_IQ_SRC_PTR = new IQSource_SoapySDR;
		G_IQ_SRC_PTR->setOption("SoapySDR_Kwargs", &device);

		if( !G_IQ_SRC_PTR->init() )
		{
			cout<<"IQSource_SoapySDR::init failed."<<endl;
			return 1;
		}

		double sr;
		G_IQ_SRC_PTR->getOption("sampling_rate_double", &sr);
		cout<<C_RED<<"SETTING SR "<<sr<<C_OFF<<endl;
		G_SAMPLES.samplingRate(sr);

		const double gain = 15;
		G_IQ_SRC_PTR->setOption("gain_double", &gain);

		G_FREQ = 434.274e6;
		SetFrequency(G_FREQ); // RTL
	}

	cout<<"G_IQ_SRC_PTR->sampling_rate() "<<G_IQ_SRC_PTR->samplingRate()<<endl;

	if( !G_IQ_SRC_PTR->start() )
	{
		cout<<"IQSource_File::start failed."<<endl;
		return 1;
	}

	if( !G_IQ_SRC_PTR->isRunning() )
	{
		cout<<"IQSource_File::isRunning failed."<<endl;
		return 1;
	}

	// DECODER
	G_DECODER.baud(baud);
	G_DECODER.rtty_bits(a_bits);
	G_DECODER.rtty_stops(a_stops);
	G_DECODER.livePrint(true);

	// std::this_thread::sleep_for( std::chrono::duration<double, std::milli>(1000) );

	std::thread* p_thread = new std::thread(RunDecoder);

	return 0;
}



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


typedef uchar LUMA_T;
typedef uchar RGB_T;
#define BG_COLOR 45
const int RES_X = 1024;
const int RES_Y = 256;

RGB_T  SPECTRUM_BITMAP_RGB 	[RES_X*RES_Y*3]; // FFT
RGB_T  TIME_BITMAP_RGB 		[RES_X*RES_Y*3]; // Demodulated samples

class FreqViewport : public Fl_Box
{
public:
	FreqViewport(int X,int Y,int W,int H, const char*L=0)
				: Fl_Box(X,Y,W,H,L), m_x(X), m_y(Y), m_res_x(W), m_res_y(H)
	{
		box(FL_FLAT_BOX);
		color(BG_COLOR);
		Fl::add_timeout(0.1, FreqViewport_CB, (void*)this);
	}

private:
	const int m_x;
	const int m_y;
	const int m_res_x;
	const int m_res_y;

	// frequency drag
	int m_xMB;
	bool m_DRAG;
	int m_xMB_x;
	int m_drag_freq_start;

	// IQVector<TReal>			m_frequencies_;
	std::vector<TReal>		m_magnitudesArr;

	int handle(int event)
	{
		if(event == FL_MOUSEWHEEL)
		{
			size_t decimation_factor = G_DECODER.getDecimationFactor();
			size_t new_decim_exp = log2(decimation_factor) - Fl::event_dy();
			cout<<"Decimation: "<<pow(2,new_decim_exp)<<endl;
			G_DECODER.setupDecimationStagesFactor(pow(2,new_decim_exp));
			return 1;
		}
		if(event ==FL_PUSH)
		{
			m_xMB = Fl::event_button();
			m_xMB_x = Fl::event_x();
			m_drag_freq_start = G_FREQ;
			return 1;
		}
		if(event == FL_RELEASE)
		{
			m_xMB = 0;
			return 1;
		}
		if(m_xMB && event == FL_DRAG)
		{
			double _dx_0_1 = double(Fl::event_x()-m_xMB_x) / w();

			double freq_change = _dx_0_1 * G_DECODER.getDecimatedSamplingRate();
			if(m_xMB != FL_LEFT_MOUSE)
				freq_change *= 0.1;

			double _f_new = double(m_drag_freq_start) - freq_change;
			cout<<_f_new<<endl;
			SetFrequency(_f_new);

			return 1;
		}
		return 0;
	}

	void draw()
	{
		SpectrumInfo<TReal> spectr_inf = G_DECODER.getSpectrumInfo();
		if(!spectr_inf.size())
			return;
		m_magnitudesArr = spectr_inf;

		if( !m_magnitudesArr.size() )
		{
			cout<<"if( !m_magnitudesArr.size() )"<<endl;
			return;
		}

		const double resize = double(m_res_x) / spectr_inf.size();

		int pl = resize * spectr_inf.peak_left_ ;
		int pr = resize * spectr_inf.peak_right_;
		double nf = spectr_inf.noise_floor_;
		double nv = spectr_inf.noise_variance_;
		SimpleDownsampleVector(m_magnitudesArr, 1.0/resize);

		static Average<double> low_level(25, spectr_inf.min_);
		low_level.add(spectr_inf.min_);
		// double low_level = -300;

		nf = (nf > (double)low_level) ? nf : (double)low_level;

		ClearBitmap(SPECTRUM_BITMAP_RGB, m_res_x, m_res_y, 3);
		Histogram2Bitmap(SPECTRUM_BITMAP_RGB, m_res_x, m_res_y,
						m_magnitudesArr,
						low_level, 0, 0,
						.0, .99, true);


		if(spectr_inf.peak_left_valid_)
			VerticalLineToBitmap(SPECTRUM_BITMAP_RGB, m_res_x, m_res_y, pl, true, 200, 50, 0);
		else
			VerticalLineToBitmap(SPECTRUM_BITMAP_RGB, m_res_x, m_res_y, pl, true, 50, 12, 0);
		if(spectr_inf.peak_right_valid_)
			VerticalLineToBitmap(SPECTRUM_BITMAP_RGB, m_res_x, m_res_y, pr, true, 0, 80, 250);
		else
			VerticalLineToBitmap(SPECTRUM_BITMAP_RGB, m_res_x, m_res_y, pr, true, 0, 20, 60);

		HorizontalLineToBitmap(SPECTRUM_BITMAP_RGB, m_res_x, (nf/low_level)*m_res_y, true, 80,80,80);
		HorizontalLineToBitmap(SPECTRUM_BITMAP_RGB, m_res_x, ((nf+nv)/low_level)*m_res_y, true, 80,80,80);


		Fl_Box::draw();
		fl_draw_image(SPECTRUM_BITMAP_RGB, m_x, m_y, m_res_x, m_res_y, 3, 0);
	}

	static void FreqViewport_CB(void *userdata)
	{
		((FreqViewport*)userdata)->redraw();
		Fl::repeat_timeout(1.0f/500, FreqViewport_CB, userdata);
	}
};


class DemodViewport : public Fl_Box
{

public:
	const int m_x;
	const int m_y;
	const int m_res_x;
	const int m_res_y;

	DemodViewport(int X,int Y,int W,int H, const char*L=0)
				: Fl_Box(X,Y,W,H,L), m_x(X), m_y(Y), m_res_x(W), m_res_y(H)
	{
		box(FL_FLAT_BOX);
		color(BG_COLOR);
		Fl::add_timeout(0.1, DemodViewport_CB, (void*)this);
	}


private:
	std::vector<TReal>	samples_;
	size_t samples_to_show_cnt_ = 0;

	void draw()
	{
		if(!samples_to_show_cnt_)
			samples_to_show_cnt_ = G_DECODER.getDecimatedSamplingRate() / G_DECODER.getSymbolRate() * 10; // 10 symbols

		const size_t samples_size_pre_insert_ = samples_.size();
		auto demod = G_DECODER.getDemodulated();
		samples_.insert(
			samples_.end(),
			demod.cbegin(),
			demod.cbegin() + min(samples_to_show_cnt_,demod.size())
				);

		// if(!samples_.size())
		if(samples_.size() < samples_to_show_cnt_)
			return;

		ClearBitmap(TIME_BITMAP_RGB, m_res_x, m_res_y, 3);
		VectorToBitmap(TIME_BITMAP_RGB, samples_, m_res_x, m_res_y);
		HorizontalLineToBitmap(TIME_BITMAP_RGB, m_res_x, m_res_y/2, true);

		Fl_Box::draw();
		fl_draw_image(TIME_BITMAP_RGB, m_x, m_y, m_res_x, m_res_y, 3, 0);

		if(samples_.size() > samples_to_show_cnt_)
			samples_.erase( samples_.begin(), samples_.end()-samples_size_pre_insert_);

	}

	static void DemodViewport_CB(void *userdata)
	{
		((DemodViewport*)userdata)->redraw();
		Fl::repeat_timeout(1.0f/10, DemodViewport_CB, userdata);
	}


};
/////////////////////////////////
/////////////////////////////////


void DO_AFC_CB(Fl_Widget* p_w, void* p_user)
{
	DO_CORRECTION = !DO_CORRECTION;
}

int GUI_MAIN()
{
	Fl::visual(FL_RGB); // disable dithering

	Fl_Double_Window* p_window = new Fl_Double_Window(RES_X, 1.5*RES_Y + 50);

	p_window->begin();
		FreqViewport* p_freqVP = new FreqViewport( 0, 0*RES_Y, 	RES_X, RES_Y);
		DemodViewport* p_timeVP = new DemodViewport( 0, 1*RES_Y, 	RES_X, RES_Y/2);
		auto button = new Fl_Button(0, 1*RES_Y + RES_Y/2 + 10, 100, 20, "AFC");
		button->callback( ( Fl_Callback* ) DO_AFC_CB );
	p_window->end();
	p_window->show(0, 0);

	return Fl::run();
}



int main(int argc, char** argv)
{
	using namespace std;

	struct thousand_separators : numpunct<char> {
	   char do_thousands_sep() const { return ','; }
	   string do_grouping() const { return "\3"; }
	};
	cout.imbue( locale(locale(""), new thousand_separators) );


	string file("");
	if(argc > 4)
		file = string(argv[4]);
	cout<<"File: "<<file<<endl;

	InitProcess( std::stoi(argv[1]), std::stoi(argv[2]), std::stoi(argv[3]), file);

	GUI_MAIN();
	// while(1)		this_thread::sleep_for(chrono::duration<double, milli>(3000));

}
