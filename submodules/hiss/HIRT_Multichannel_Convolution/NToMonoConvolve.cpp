

#include "NToMonoConvolve.h"

#include <string.h>

HISSTools::DSP::NToMonoConvolve::NToMonoConvolve(uint32_t input_chans, uintptr_t maxLength, t_convolve_latency_mode latency) :  mNumInChans(input_chans)
{
    for (uint32_t i = 0; i < mNumInChans; i++)
        mConvolvers.push_back(new MonoConvolve(maxLength, latency));
}

HISSTools::DSP::NToMonoConvolve::~NToMonoConvolve()
{
    for (uint32_t i = 0; i < mNumInChans; i++)
        delete mConvolvers[i];
}

t_convolve_error HISSTools::DSP::NToMonoConvolve::resize(uint32_t inChan, uintptr_t impulse_length)
{
	if (inChan < mNumInChans)
	{
		if (!mConvolvers[inChan]->resize(impulse_length, false))
			return CONVOLVE_ERR_MEM_UNAVAILABLE;
	}
	else
		return CONVOLVE_ERR_IN_CHAN_OUT_OF_RANGE;

	return CONVOLVE_ERR_NONE;
}

t_convolve_error HISSTools::DSP::NToMonoConvolve::set(uint32_t inChan, const float *input, uintptr_t impulse_length, bool resize)
{
	if (inChan < mNumInChans)
		return mConvolvers[inChan]->set(input, impulse_length, resize);
	else
		return CONVOLVE_ERR_IN_CHAN_OUT_OF_RANGE;
}

t_convolve_error HISSTools::DSP::NToMonoConvolve::reset(uint32_t inChan)
{
    if (inChan < mNumInChans)
        return mConvolvers[inChan]->reset();
    else
        return CONVOLVE_ERR_IN_CHAN_OUT_OF_RANGE;
}

void HISSTools::DSP::NToMonoConvolve::process(float **ins, float *out, float *temp1, uintptr_t numSamples, uint32_t active_inChans)
{
    // Zero output
    
    memset(out, 0, sizeof(float) * numSamples);

	// Convolve
	
	for (int i = 0; i < mNumInChans && i < active_inChans ; i++)
		mConvolvers[i]->process(ins[i], temp1, out, numSamples);
}

void HISSTools::DSP::NToMonoConvolve::process(float **ins, double *out, float *temp1, float *temp2, uintptr_t numSamples, uint32_t active_inChans)
{
	// Zero out temp
		
    memset(temp2, 0, sizeof(float) * numSamples);

    // Convolve
	
	for (uint32_t i = 0; i < mNumInChans && i < active_inChans ; i++)
		mConvolvers[i]->process(ins[i], temp1, temp2, numSamples);
	
	// Copy output
	
	for (AH_UIntPtr i = 0; i < numSamples; i++)
		out[i] = temp2[i];
}