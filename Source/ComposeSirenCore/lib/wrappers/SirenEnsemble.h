//
// Created by joseph larralde on 17/02/2026.
//

#ifndef COMPOSESIREN_SIREN_ENSEMBLE_H
#define COMPOSESIREN_SIREN_ENSEMBLE_H

#include "SirenVoice.h"
#include "../dsp/rampe.h"
#include "../definitions/parameterDefinitions.h"

const float pi_2 = 1.57079632679489661923f;

//------------------------------------------------------------------------------
// SIREN TRACK /////////////////////////////////////////////////////////////////

// shall we add integrated smooth filters for pan/gain controls ?
class SirenTrack : public SirenVoice
{
    std::atomic<float> gain{1.0f};    // identity
    float g{0.0f};
    rampe<float> gr{0, 10};

    std::atomic<float> panning{0.0f}; // centred
    float p{0.0f};
    rampe<float> pr{1, 10};

    float lp{0.0f};
    float rp{0.0f};

public:
    SirenTrack(sirenId id, const std::string& resourcesPath);
    ~SirenTrack() override = default;

    void setSampleRate(double sr) override;
    void setPanning(float p);
    void setOutputGain(float g);

    // call this at the beginning of the audio callback
    void beginProcessBlock() override;

    // call this for each sample during the audio callback
    void process(float *l, float* r);

private:
    // we can add sqrt panning law and others later if we like
    void computePanningGains();
};

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// SIREN ENSEMBLE //////////////////////////////////////////////////////////////

class SirenEnsemble {
public:
    SirenEnsemble(const std::vector<sirenId>& ids,
                  const std::string& resourcesPath);
    ~SirenEnsemble() = default;

    void updateResourcesPath(const std::string& resourcesPath);
    bool areSirensLoading() const;
    bool getRawSirenHandles() const;
    std::vector<sirenId> getSirenIds() const;

    void addListener(SirenVoice::Listener* listener);
    void removeListener(SirenVoice::Listener* listener);
    void removeAllListeners();
    void notifyListeners();

    void update();
    void setSampleRate(double sr);
    void stop(std::optional<sirenId> id = std::nullopt);

    void setPanning(sirenId id, float p);
    void setOutputGain(sirenId id, float g);
    // bool getIsNoteOn(sirenId id);
    // float getCurrentPitch(sirenId);
    void setMasterVolume(float v);
    void handleMidi(int status, int value1, int value2);
    void beginProcessBlock();
    void process(float* l, float* r);
    void deleteDiscarded() const;

private:
    std::atomic<float> masterVolume = 1.0f;
    rampe<float> vr{1, 10};
    float v{0};

    std::map<sirenId, std::unique_ptr<SirenTrack>> tracks;
};

//------------------------------------------------------------------------------

#endif //COMPOSESIREN_SIREN_ENSEMBLE_H
