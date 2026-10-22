#include "log/log.h"
#include <cutils/log.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "eas_options.h"
#include "eas_wtengine.h"
#include "eas_report.h"
#include "eas_audioconst.h"

#if defined(_FILTER_ENABLED) && defined(_FLOAT_DCF)

/*----------------------------------------------------------------------------
 * WT_VoiceFilter
 *----------------------------------------------------------------------------
 * Purpose:
 * Implements a 2-pole IIR filter
 *
 * Inputs:
 *
 * Outputs:
 *
 *----------------------------------------------------------------------------
*/
void WT_VoiceFilter (S_FILTER_CONTROL *pFilter, S_WT_INT_FRAME *pWTIntFrame)
{
    if (pWTIntFrame->frame.b02 == 0) {
        return;
    }

    /* initialize some local variables */
    EAS_I32 numSamples = pWTIntFrame->numSamples;
    if (numSamples <= 0) {
        EAS_Report(_EAS_SEVERITY_ERROR, "%s: numSamples <= 0\n", __func__);
        ALOGE("b/26366256");
        android_errorWriteLog(0x534e4554, "26366256");
        return;
    } else if (numSamples > BUFFER_SIZE_IN_MONO_SAMPLES) {
        EAS_Report(_EAS_SEVERITY_ERROR, "%s: numSamples %d > %d BUFFER_SIZE_IN_MONO_SAMPLES\n", __func__, numSamples, BUFFER_SIZE_IN_MONO_SAMPLES);
        ALOGE("b/317780080 clip numSamples %d -> %d", numSamples, BUFFER_SIZE_IN_MONO_SAMPLES);
        android_errorWriteLog(0x534e4554, "317780080");
        numSamples = BUFFER_SIZE_IN_MONO_SAMPLES;
    }
    EAS_PCM *pAudioBuffer = pWTIntFrame->pAudioBuffer;

    // General 2-pole IIR transfer function is:
    //  H(z) = (b0 + b1 * z^-1 + b2 * z^-2) / (1 + a1 * z^-1 + a2 * z^-2)
    // Its difference equation is:
    //  y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] - a1 * y[n-1] - a2 * y[n-2]
    const float b1 = pWTIntFrame->frame.b1;
    const float b02 = pWTIntFrame->frame.b02;
    const float a1 = pWTIntFrame->frame.a1;
    const float a2 = pWTIntFrame->frame.a2;

    EAS_I32 y1 = pFilter->y1; // y[n-1]
    EAS_I32 y2 = pFilter->y2; // y[n-2]
    EAS_I32 x1 = pFilter->x1; // x[n-1]
    EAS_I32 x2 = pFilter->x2; // x[n-2]

    const float limit = 1 << 15;

    while (numSamples--)
    {
        float acc0 = b02 * (*pAudioBuffer) + b1 * x1 + b02 * x2 - a1 * y1 - a2 * y2;

        // saturate
        if (acc0 > limit - 1) acc0 = limit - 1;
        if (acc0 < -limit) acc0 = -limit;

        y2 = y1;
        y1 = (EAS_I32) acc0;
        x2 = x1;
        x1 = *pAudioBuffer;

        *pAudioBuffer++ = y1; // y[n]
    }

    /* save delay values */
    pFilter->y1 = y1;
    pFilter->y2 = y2;
    pFilter->x1 = x1;
    pFilter->x2 = x2;
}

/* 
 * Compute filter coefficients for a 2nd-order (2-pole) low-pass IIR filter
 *
 * General 2-pole IIR transfer function is:
 *  H(z) = (b0 + b1 * z^-1 + b2 * z^-2) / (1 + a1 * z^-1 + a2 * z^-2)
 * Its difference equation is:
 *  y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] - a1 * y[n-1] - a2 * y[n-2]
 *
 */
void WT_SetFilterCoeffs(S_WT_INT_FRAME *pIntFrame, EAS_I32 cutoff, EAS_I32 resonance) {
    double fc = pow(2.0, (cutoff - 6900.0) / 1200.0) * 440.0;

    const double fs = (double)_OUTPUT_SAMPLE_RATE;
    const double min_fc = fs / 240.0;
    const double max_fc = fs / 2.0;
    fc = fmax(fmin(fc, max_fc), min_fc);

    // EAS's resonance is in 0.75dB steps
    const double resonance_dB = fmax(resonance / 10.0, 0.0);

    // filter pole angle
    const double theta = 2.0 * M_PI * fc / fs;

    const double T = 1.0 / _OUTPUT_SAMPLE_RATE;
    const double omega0 = 2.0 * _OUTPUT_SAMPLE_RATE * tan(theta / 2);
    double q;
    if (resonance_dB < 1e-9) {
        q = 1 / sqrt(2); // default Q for Butterworth filter
    } else {
        q = pow(10.0, resonance_dB / 20.0);
    }

    const double omega0T = omega0 * T;
    const double D = 4 + 2 * omega0T / q + omega0T * omega0T;

    // compute filter coefficients
    const double a1 = (-8 + 2 * (omega0T * omega0T)) / D;
    const double a2 = (4 - 2 * omega0T / q + omega0T * omega0T) / D;
    double b02 = omega0T * omega0T / D;
    double b1 = 2 * (omega0T * omega0T) / D;

    // apply resonance gain compensation
    const double g = pow(10.0, -resonance_dB / 40.0);
    b02 *= g;
    b1 *= g;

    pIntFrame->frame.b1 = b1;
    pIntFrame->frame.b02 = b02;
    pIntFrame->frame.a1 = a1;
    pIntFrame->frame.a2 = a2;
}
#endif
