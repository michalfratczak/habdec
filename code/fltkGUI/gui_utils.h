#pragma once

#include <cstring>
#include <cmath>
#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>

#define BG_COLOR 45
#define TICK_COLOR 50
#define CIRC_COLOR 0

typedef uchar LUMA_T;
typedef uchar RGB_T;

extern const int RES_X;
extern const int RES_Y;

extern RGB_T  SPECTRUM_BITMAP_RGB 	[];
extern RGB_T  TIME_BITMAP_RGB 		[];


template<class T>
void SimpleDownsampleVector(std::vector<T>& vec, size_t factor)
{
	using namespace std;

	if(!vec.size())
		return;

	if(factor<2)
		return;

	const size_t _S = vec.size() / factor;

	if(factor == 2)
	{
		for(size_t i=0; i<_S; ++i)
			vec[i] = .5 * (vec[2*i] + vec[2*i+1]);
	}
	else
	{	//somehow does not work for ractor=2
		for(size_t i=0; i<_S; ++i)
		{
			for(size_t j=1; j<factor; ++j)
				vec[i] += vec[i*factor + j];

			vec[i] /= factor;
		}
	}

	vec.resize(_S);
}




void ClearBitmap(LUMA_T* p_bitmap, int x, int y, int s=1)
{
	memset( (void*)p_bitmap, BG_COLOR, x*y * s * sizeof(LUMA_T) );
}


template<typename COLOR_T>
void Histogram2Bitmap(COLOR_T* p_bitmap, int spectr_res_x, int spectr_res_y,
							//float* p_values, int num_values,
							const std::vector<float>& values,
							float min_value, float max_value, float avg_value,
							float offset,  float scale,
							bool is_rgb)
{
	using namespace std;

	if(min_value == max_value)
	{
		min_value = *min_element( values.begin(), values.end() );
		max_value = *max_element( values.begin(), values.end() );
	}

	//if(fabs(min_value) > 100 || fabs(max_value) > 100)		return;

	static float total_scale = fabs(max_value - min_value);
	//cout<<"total_scale "<<total_scale<<endl;

	for(int pix_x=0; pix_x<spectr_res_x; ++pix_x)
	{
		int i = round( float(pix_x) / (spectr_res_x-1) * (values.size()-1) );

		//cout<<i<<" values[i] "<<values[i]<<endl;

		// input values are log10, so can be negatives
		float val_0_1 = (values[i] - min_value) / total_scale;

		val_0_1 = offset + scale * val_0_1;

		if(val_0_1 < 0)			val_0_1 = 0;
		if(val_0_1 > 1)			val_0_1 = 1;

		int y_tip = val_0_1 * (spectr_res_y-1);
		y_tip = spectr_res_y - 1 - y_tip;

		int x = pix_x;
		for(int y=y_tip; y<spectr_res_y; ++y)
		{
			//pixel brightness
			float _y_0_1 = 1.0f - (float(y) / spectr_res_y);;
			float _b = 10.0f * _y_0_1 * val_0_1;
			if(_b < 0)			_b = 0;
			if(_b > 1)			_b = 1;

			size_t off = y*spectr_res_x+x;
			if(is_rgb)
			{
				off *= 3;

				if(pix_x > .5f * spectr_res_x)
				{
					p_bitmap[off] = (COLOR_T)(_b * 255);
					p_bitmap[off+1] = (COLOR_T)(_b * 0);
					p_bitmap[off+2] = 0;
				}
				else
				{
					p_bitmap[off] =  0;
					p_bitmap[off+1] = (COLOR_T)(_b * 155);
					p_bitmap[off+2] = (COLOR_T)(_b * 255);
				}
			}
			else
			{
				p_bitmap[off] = (COLOR_T)(_b * 255);//220;
			}
		}
	}

	// center vertical line
	for(int y=0; y<spectr_res_y; ++y)
	{
		size_t off = y*spectr_res_x+spectr_res_x/2;
		if(is_rgb)
		{
			off *= 3;
			p_bitmap[off] = (COLOR_T)(110);
			p_bitmap[off+1] = (COLOR_T)(110);
			p_bitmap[off+2] = 110;
		}
		else
		{
			p_bitmap[off] = (COLOR_T)(110);//220;
		}
	}
}


template<typename COLOR_T, typename TReal>
void VectorToBitmap(COLOR_T* p_bitmap, const std::vector<TReal>& samples_, int m_res_x, int m_res_y)
{
	auto abs_sum = [&](TReal a, TReal b){return abs(a) + abs(b);};
	const TReal avg = accumulate(samples_.begin(), samples_.end(), .0, abs_sum) / samples_.size();

	for(int x=0; x<m_res_x; ++x)
	{
		TReal _x_0_1 = TReal(x) / m_res_x;
		int i = _x_0_1 * samples_.size();

		TReal val_rel = samples_[i] / (3*avg);
		val_rel = min(max(-1.0f,val_rel), 1.0f);
		val_rel = .5 + .5 * val_rel;

		int y = (1.0f - val_rel) * m_res_y;
		if(y >= m_res_y)
			y = m_res_y - 1;

		size_t off = y*m_res_x+x;
		if(samples_[_x_0_1 * samples_.size()] > 0)
		{
			p_bitmap[3*off+0] = (RGB_T)(255);
			p_bitmap[3*off+1] = (RGB_T)(0);
			p_bitmap[3*off+2] = (RGB_T)(0);
		}
		else
		{
			p_bitmap[3*off+0] = (RGB_T)(0);
			p_bitmap[3*off+1] = (RGB_T)(150);
			p_bitmap[3*off+2] = (RGB_T)(255);
		}
	}
}


template<typename COLOR_T>
void HorizontalLineToBitmap(COLOR_T* p_bitmap, int spectr_res_x, int y, bool is_rgb,
		short r=0, short g=200, short b=250)
{
	for(int x=0; x<spectr_res_x; ++x)
	{
		size_t off = y*spectr_res_x+x;
		if(is_rgb)
		{
			off *= 3;
			p_bitmap[off] = r;
			p_bitmap[off+1] = g;
			p_bitmap[off+2] = b;
		}
		else
		{
			p_bitmap[off] = (int(r)+g+b)/3;
		}
	}
}


template<typename COLOR_T>
void VerticalLineToBitmap(COLOR_T* p_bitmap, int spectr_res_x, int spectr_res_y, int x, bool is_rgb,
		short r=0, short g=200, short b=250)
{
	//int x = peak.first;
	for(int y=0; y<spectr_res_y; ++y)
	{
		size_t off = y*spectr_res_x+x;
		if(is_rgb)
		{
			off *= 3;
			p_bitmap[off] = r;
			p_bitmap[off+1] = g;
			p_bitmap[off+2] = b;
		}
		else
		{
			p_bitmap[off] = (int(r)+g+b)/3;
		}
	}
}
