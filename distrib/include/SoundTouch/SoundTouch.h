/*****************************************************************************
 *
 * A main class for tempo/pitch/rate adjusting routines. 
 *
 * Initialize the object by setting up the sound stream parameters with the
 * 'setSampleRate' and 'setChannels' functions, and set the desired
 * tempo/pitch/rate settings with the corresponding functions.
 *
 * Notes:
 * * This class behaves like an first-in-first-out pipe: The samples to be 
 *  processed are fed into one of the pipe with the 'putSamples' function,
 *  and the ready, processed samples are read from the other end with the 
 *  'receiveSamples' function. 
 *
 * * The tempo/pitch/rate control parameters may freely be altered during 
 * processing.
 *
 * * The processing routines introduce a certain 'latency' between the
 * input and output, so that the inputted samples aren't immediately 
 * transferred to the output, and neither the number of output samples 
 * immediately available after inputting some samples isn't in direct 
 * relationship to the number of previously inputted samples.
 *
 * * This class utilizes classes 'tempochanger' to change tempo of the
 * sound (without changing pitch) and 'transposer' to change rate
 * (that is, both tempo and pitch) of the sound. The third available control 
 * 'pitch' (change pitch but maintain tempo) is produced by suitably 
 * combining the two preceding controls.
 *
 * Author        : Copyright (c) Olli Parviainen
 * Author e-mail : oparviai @ iki.fi
 *
 * Last changed  : $Date: 2003/12/27 11:54:16 $
 * File revision : $Revision: 1.7 $
 *
 * $Id: SoundTouch.h,v 1.7 2003/12/27 11:54:16 Olli Exp $
 *
 * License :
 *
 *  SoundTouch sound processing library
 *  Copyright (c) Olli Parviainen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************/

#ifndef SoundTouch_H
#define SoundTouch_H

#include "FIFOSamplePipe.h"
#include "STTypes.h"

/// Soundtouch library version string
#define SOUNDTOUCH_VERSION          "1.2.1"

/// SoundTouch library version id
#define SOUNDTOUCH_VERSION_ID       010201

//
// Available setting IDs for the 'setSetting' & 'get_setting' functions:

/// Enable/disable anti-alias filter in pitch transposer (0 = disable)
#define SETTING_USE_AA_FILTER       0

/// Pitch transposer anti-alias filter length (8 .. 128 taps, default = 32)
#define SETTING_AA_FILTER_LENGTH    1

/// Enable/disable quick seeking algorithm in tempo changer routine
/// (enabling quick seeking lowers CPU utilization but causes a minor sound
///  quality compromising)
#define SETTING_USE_QUICKSEEK       2

/// Time-stretch algorithm single processing sequence length in milliseconds. This determines 
/// to how long sequences the original sound is chopped in the time-stretch algorithm. 
/// See "STTypes.h" or README for more information.
#define SETTING_SEQUENCE_MS         3

/// Time-stretch algorithm seeking window length in milliseconds for algorithm that finds the 
/// best possible overlapping location. This determines from how wide window the algorithm 
/// may look for an optimal joining location when mixing the sound sequences back together. 
/// See "STTypes.h" or README for more information.
#define SETTING_SEEKWINDOW_MS       4

/// Time-stretch algorithm overlap length in milliseconds. When the chopped sound sequences 
/// are mixed back together, to form a continuous sound stream, this parameter defines over 
/// how long period the two consecutive sequences are let to overlap each other. 
/// See "STTypes.h" or README for more information.
#define SETTING_OVERLAP_MS          5


class SoundTouch : public FIFOProcessor
{
private:
    /// Rate transposer class instance
    class RateTransposer *pRateTransposer;

    /// Time-stretch class instance
    class TDStretch *pTDStretch;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    float virtualRate;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    float virtualTempo;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    float virtualPitch;

    /// Flag: Has sample rate been set?
    BOOL  bSrateSet;

    /// Calculates effective rate & tempo valuescfrom 'virtualRate', 'virtualTempo' and 
    /// 'virtualPitch' parameters.
    void calcEffectiveRateAndTempo();

protected :
    /// Number of channels
    uint  channels;

    /// Effective 'rate' value calculated from 'virtualRate', 'virtualTempo' and 'virtualPitch'
    float rate;

    /// Effective 'tempo' value calculated from 'virtualRate', 'virtualTempo' and 'virtualPitch'
    float tempo;

public:
    SoundTouch();
    virtual ~SoundTouch();

    /// Get SoundTouch library version string
    static const char *getVersionString();

    /// Get SoundTouch library version Id
    static uint SoundTouch::getVersionId();

    /// Sets new rate control value. Normal rate = 1.0, smaller values
    /// represent slower rate, larger faster rates.
    void setRate(float newRate);

    /// Sets new tempo control value. Normal tempo = 1.0, smaller values
    /// represent slower tempo, larger faster tempo.
    void setTempo(float newTempo);

    /// Sets new rate control value as a difference in percents compared
    /// to the original rate (-50 .. +100 %)
    void setRateChange(float newRate);

    /// Sets new tempo control value as a difference in percents compared
    /// to the original tempo (-50 .. +100 %)
    void setTempoChange(float newTempo);

    /// Sets new pitch control value. Original pitch = 1.0, smaller values
    /// represent lower pitches, larger values higher pitch.
    void setPitch(float newPitch);

    /// Sets pitch change in octaves compared to the original pitch  
    /// (-1.00 .. +1.00)
    void setPitchOctaves(float newPitch);

    /// Sets pitch change in semi-tones compared to the original pitch
    /// (-12 .. +12)
    void setPitchSemiTones(int newPitch);
    void setPitchSemiTones(float newPitch);

    /// Sets the number of channels, 1 = mono, 2 = stereo
    void setChannels(uint numChannels);

    /// Sets sample rate.
    void setSampleRate(uint srate);

    /// Flushes the last samples from the processing pipeline to the output.
    /// Clears also the internal processing buffers.
    //
    /// Note: This function is meant for extracting the last samples of a sound
    /// stream. This function may introduce additional blank samples in the end
    /// of the sound stream, and thus it's not recommended to call this function
    /// in the middle of a sound stream.
    void flush();

    /// Adds 'numSamples' pcs of samples from the 'samples' memory position into
    /// the input of the object. Notice that sample rate _has_to_ be set before
    /// calling this function, otherwise throws a runtime_error exception.
    virtual void putSamples(
            const soundtouch::SAMPLETYPE *samples,  ///< Pointer to sample buffer.
            uint numSamples                         ///< Number of samples in buffer. Notice
                                                    ///< that in case of stereo-sound a single sample
                                                    ///< contains data for both channels.
            );

    /// Clears all the samples in the object's output and internal processing
    /// buffers.
    virtual void clear();

    /// Changes a setting controlling the processing system behaviour. See the
    /// 'SETTING_...' defines for available setting ID's.
    /// 
    /// \return 'TRUE' if the setting was succesfully changed
    BOOL setSetting(uint settingId,   ///< Setting ID number. see SETTING_... defines.
                    uint value        ///< New setting value.
                    );

    /// Reads a setting controlling the processing system behaviour. See the
    /// 'SETTING_...' defines for available setting ID's.
    ///
    /// \return the setting value.
    uint getSetting(uint settingId    ///< Setting ID number, see SETTING_... defines.
                    ) const;

};

#endif
