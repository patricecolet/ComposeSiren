//
// Created by joseph larralde on 06/05/2026.
//

#ifndef COMPOSESIREN_SIRENSTATEMONITOR_H
#define COMPOSESIREN_SIRENSTATEMONITOR_H

#include "SirenEnsemble.h"

// NB :
// improvement -> constructor could have a
// std::variant<SirenVoice*, SirenEnsemble*> argument
// and execute subscribe directly
class SirenStateMonitor : public SirenVoice::Listener
{
    std::variant<std::monostate, SirenVoice*, SirenEnsemble*> notifier;
    std::vector<sirenId> activeSirens;

public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void activeSirenIds(const std::vector<sirenId>&) {}
        virtual void currentSirenState(const sirenId, const SirenVoice::State&) {}
    };

private:
    std::set<Listener*> listeners;

public:
    SirenStateMonitor() = default;
    ~SirenStateMonitor() override {
        unsubscribe();
        listeners.clear();
    }

    void addListener(Listener* l) { listeners.insert(l); }
    void removeListener(Listener* l) { listeners.erase(l); }
    void removeAllListeners() { listeners.clear(); }

    void subscribe(SirenVoice* s) {
        unsubscribe();
        notifier = s;
        s->addListener(this);

        sirenId id;
        if (s->getSirenId(id)) { activeSirens = { id }; }
        else { activeSirens = {}; }
    }

    void subscribe(SirenEnsemble* s) {
        unsubscribe();
        notifier = s;
        s->addListener(this);

        activeSirens = s->getSirenIds();
    }

    void unsubscribe() {
        if (const auto* sv = std::get_if<SirenVoice*>(&notifier)) {
            (*sv)->removeListener(this);
        } else if (const auto* se = std::get_if<SirenEnsemble*>(&notifier)) {
            (*se)->removeListener(this);
        }
        notifier = std::monostate{};
    }

    [[nodiscard]] const std::vector<sirenId>& getActiveSirenIds() const {
        return activeSirens;
    }

    // SirenVoice callbacks ----------------------------------------------------
    void currentSirenId(const sirenId id) override {
        if (const auto* sv = std::get_if<SirenVoice*>(&notifier)) {
            activeSirens = { id };
            for (auto* l : listeners) { l->activeSirenIds(activeSirens); }
        } else {
            assert(false); // error case, ids never change in SirenEnsemble
        }
    }

    void currentSirenState(const sirenId id,
                           const SirenVoice::State state) override {
        for (auto* l : listeners) { l->currentSirenState(id, state); }
        // std::cout << "sending siren state " << id << " " << state.currentPitch << std::endl;
    }
};

#endif // COMPOSESIREN_SIRENSTATEMONITOR_H