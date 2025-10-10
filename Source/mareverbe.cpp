/*
 *  mareverbe.cpp
 *  S1IN
 *
 *  Created by benoit louette on 19/03/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "mareverbe.h"
#include <iostream>

mareverbe::mareverbe()
{
	// Tie the components to their buffers
	combL[0].setbuffer(bufcombL1,combtuningL1);
	combR[0].setbuffer(bufcombR1,combtuningR1);
	combL[1].setbuffer(bufcombL2,combtuningL2);
	combR[1].setbuffer(bufcombR2,combtuningR2);
	combL[2].setbuffer(bufcombL3,combtuningL3);
	combR[2].setbuffer(bufcombR3,combtuningR3);
	combL[3].setbuffer(bufcombL4,combtuningL4);
	combR[3].setbuffer(bufcombR4,combtuningR4);
	combL[4].setbuffer(bufcombL5,combtuningL5);
	combR[4].setbuffer(bufcombR5,combtuningR5);
	combL[5].setbuffer(bufcombL6,combtuningL6);
	combR[5].setbuffer(bufcombR6,combtuningR6);
	combL[6].setbuffer(bufcombL7,combtuningL7);
	combR[6].setbuffer(bufcombR7,combtuningR7);
	combL[7].setbuffer(bufcombL8,combtuningL8);
	combR[7].setbuffer(bufcombR8,combtuningR8);
	allpassL[0].setbuffer(bufallpassL1,allpasstuningL1);
	allpassR[0].setbuffer(bufallpassR1,allpasstuningR1);
	allpassL[1].setbuffer(bufallpassL2,allpasstuningL2);
	allpassR[1].setbuffer(bufallpassR2,allpasstuningR2);
	allpassL[2].setbuffer(bufallpassL3,allpasstuningL3);
	allpassR[2].setbuffer(bufallpassR3,allpasstuningR3);
	allpassL[3].setbuffer(bufallpassL4,allpasstuningL4);
	allpassR[3].setbuffer(bufallpassR4,allpasstuningR4);
	
	// Set default values
	allpassL[0].setfeedback(0.5f);
	allpassR[0].setfeedback(0.5f);
	allpassL[1].setfeedback(0.5f);
	allpassR[1].setfeedback(0.5f);
	allpassL[2].setfeedback(0.5f);
	allpassR[2].setfeedback(0.5f);
	allpassL[3].setfeedback(0.5f);
	allpassR[3].setfeedback(0.5f);
	setwet(initialwet);
	setroomsize(initialroom);
	setdry(initialdry);
	setdamp(initialdamp);
	setwidth(initialwidth);
	setmode(initialmode);
	
	// Buffer will be full of rubbish - so we MUST mute them
	mute();
	
}

mareverbe::~mareverbe() {
}

void mareverbe::process_events() {
	if (roomsize)
		setroomsize(roomsize);
	if (damp)
		setdamp(damp);
	if (wet)
		setwet(dbtoamp(wet,-48.0f));
	if (dry)
		setdry(dbtoamp(dry,-48.0f));
	if (width)
		setwidth(width);
	if (freeze)
		setmode(freeze);
}

void mareverbe::process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {
	processreplace(inL, inR, outL, outR, n, 1);
	dsp_clip(outL, n, 1);
	dsp_clip(outR, n, 1); // signal may never exceed -1..1
	
}

void mareverbe::mute()
{
	int i;
	
	if (getmode() >= freezemode)
		return;
	
	for (i=0;i<numcombs;i++)
	{
		combL[i].mute();
		combR[i].mute();
	}
	for (i=0;i<numallpasses;i++)
	{
		allpassL[i].mute();
		allpassR[i].mute();
	}
}

void mareverbe::processreplace(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip)
{
	float outL,outR,input;
	int i;
	input = (*inputL + *inputR) * gain;
	if((input <=1.0)&&(input >=-1.0)){ 
		
	while(numsamples--)
	{
		outL = outR = 0;
		input = (*inputL + *inputR) * gain;
		
		// Accumulate comb filters in parallel
		for(i=0; i<numcombs; i++)
		{
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}
		
		// Feed through allpasses in series
		for(i=0; i<numallpasses; i++)
		{
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output REPLACING anything already there
		*outputL = outL*wet1 + outR*wet2 + (*inputL)*dry;
		*outputR = outR*wet1 + outL*wet2 + (*inputR)*dry;
		

		// Increment sample pointers, allowing for interleave (if any) 
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	
	}
	
	}
}



void mareverbe::update()
{
	// Recalculate internal values after parameter change
	
	int i;
	
	wet1 = wet*(width/2 + 0.5f);
	wet2 = wet*((1-width)/2);
	
	if (mode >= freezemode)
	{
		roomsize1 = 1;
		damp1 = 0;
		gain = muted;
	}
	else
	{
		roomsize1 = roomsize;
		damp1 = damp;
		gain = fixedgain;
	}
	
	for(i=0; i<numcombs; i++)
	{
		combL[i].setfeedback(roomsize1);
		combR[i].setfeedback(roomsize1);
	}
	
	for(i=0; i<numcombs; i++)
	{
		combL[i].setdamp(damp1);
		combR[i].setdamp(damp1);
	}
}

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void mareverbe::setroomsize(float value)
{
	roomsize = (value*scaleroom) + offsetroom;
	update();
}

float mareverbe::getroomsize()
{
	return (roomsize-offsetroom)/scaleroom;
}

void mareverbe::setdamp(float value)
{
	damp = value*scaledamp;
	update();
}

float mareverbe::getdamp()
{
	return damp/scaledamp;
}

void mareverbe::setwet(float value)
{
	wet = value*scalewet;
	update();
}

float mareverbe::getwet()
{
	return wet/scalewet;
}

void mareverbe::setdry(float value)
{
	dry = value*scaledry;
}

float mareverbe::getdry()
{
	return dry/scaledry;
}

void mareverbe::setwidth(float value)
{
	width = value;
	update();
}

float mareverbe::getwidth()
{
	return width;
}

void mareverbe::setmode(float value)
{
	mode = value;
	update();
}

float mareverbe::getmode()
{
	if (mode >= freezemode)
		return 1;
	else
		return 0;
}

