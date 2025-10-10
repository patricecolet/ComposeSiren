// Comb filter class declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#ifndef _comb_
#define _comb_

class comb
{
public:
	comb();
	void	setbuffer(float *buf, int size);
	inline float process(float input)
	{
		float output;
		output = buffer[bufidx];
		
		if(!((filterstore <=1.0)&&(filterstore >=-1.0))) filterstore=0.0;

		
		filterstore = (output*damp2) + (filterstore*damp1);
		
		buffer[bufidx] = input + (filterstore*feedback);
		
		if(++bufidx>=bufsize) bufidx = 0;

		return output;
	}
	
	void	mute();
	void	setdamp(float val);
	float	getdamp();
	void	setfeedback(float val);
	float	getfeedback();
private:
	float	feedback;
	float	filterstore;
	float	damp1;
	float	damp2;
	float	*buffer;
	int		bufsize;
	int		bufidx;
};


#endif //_comb_

//ends

