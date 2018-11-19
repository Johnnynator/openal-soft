#ifndef _AL_AUXEFFECTSLOT_H_
#define _AL_AUXEFFECTSLOT_H_

#include "alMain.h"
#include "alEffect.h"

#include "almalloc.h"
#include "atomic.h"


struct ALeffectStateVtable;
struct ALeffectslot;

typedef struct ALeffectState {
    RefCount Ref;
    const struct ALeffectStateVtable *vtbl;

    ALfloat (*OutBuffer)[BUFFERSIZE];
    ALsizei OutChannels;
} ALeffectState;

void ALeffectState_Construct(ALeffectState *state);
void ALeffectState_Destruct(ALeffectState *state);

struct ALeffectStateVtable {
    void (*const Destruct)(ALeffectState *state);

    ALboolean (*const deviceUpdate)(ALeffectState *state, ALCdevice *device);
    void (*const update)(ALeffectState *state, const ALCcontext *context, const struct ALeffectslot *slot, const union ALeffectProps *props);
    void (*const process)(ALeffectState *state, ALsizei samplesToDo, const ALfloat (*RESTRICT samplesIn)[BUFFERSIZE], ALfloat (*RESTRICT samplesOut)[BUFFERSIZE], ALsizei numChannels);

    void (*const Delete)(void *ptr);
};

/* Small hack to use a pointer-to-array types as a normal argument type.
 * Shouldn't be used directly.
 */
typedef ALfloat ALfloatBUFFERSIZE[BUFFERSIZE];

#define DEFINE_ALEFFECTSTATE_VTABLE(T)                                        \
DECLARE_THUNK(T, ALeffectState, void, Destruct)                               \
DECLARE_THUNK1(T, ALeffectState, ALboolean, deviceUpdate, ALCdevice*)         \
DECLARE_THUNK3(T, ALeffectState, void, update, const ALCcontext*, const ALeffectslot*, const ALeffectProps*) \
DECLARE_THUNK4(T, ALeffectState, void, process, ALsizei, const ALfloatBUFFERSIZE*RESTRICT, ALfloatBUFFERSIZE*RESTRICT, ALsizei) \
static void T##_ALeffectState_Delete(void *ptr)                               \
{ return T##_Delete(STATIC_UPCAST(T, ALeffectState, (ALeffectState*)ptr)); }  \
                                                                              \
static const struct ALeffectStateVtable T##_ALeffectState_vtable = {          \
    T##_ALeffectState_Destruct,                                               \
                                                                              \
    T##_ALeffectState_deviceUpdate,                                           \
    T##_ALeffectState_update,                                                 \
    T##_ALeffectState_process,                                                \
                                                                              \
    T##_ALeffectState_Delete,                                                 \
}


struct EffectStateFactory {
    virtual ~EffectStateFactory() { }

    virtual ALeffectState *create() = 0;
};


#define MAX_EFFECT_CHANNELS (4)


struct ALeffectslotArray {
    ALsizei count;
    struct ALeffectslot *slot[];
};


struct ALeffectslotProps {
    ALfloat   Gain;
    ALboolean AuxSendAuto;

    ALenum Type;
    ALeffectProps Props;

    ALeffectState *State;

    ATOMIC(struct ALeffectslotProps*) next;
};


struct ALeffectslot {
    ALfloat   Gain{1.0f};
    ALboolean AuxSendAuto{AL_TRUE};

    struct {
        ALenum Type{AL_EFFECT_NULL};
        ALeffectProps Props{};

        ALeffectState *State{nullptr};
    } Effect;

    ATOMIC(ALenum) PropsClean{AL_TRUE};

    RefCount ref{0u};

    ATOMIC(struct ALeffectslotProps*) Update{nullptr};

    struct {
        ALfloat   Gain{1.0f};
        ALboolean AuxSendAuto{AL_TRUE};

        ALenum EffectType{AL_EFFECT_NULL};
        ALeffectProps EffectProps{};
        ALeffectState *EffectState{nullptr};

        ALfloat RoomRolloff{0.0f}; /* Added to the source's room rolloff, not multiplied. */
        ALfloat DecayTime{0.0f};
        ALfloat DecayLFRatio{0.0f};
        ALfloat DecayHFRatio{0.0f};
        ALboolean DecayHFLimit{AL_FALSE};
        ALfloat AirAbsorptionGainHF{1.0f};
    } Params;

    /* Self ID */
    ALuint id{};

    ALsizei NumChannels{};
    BFChannelConfig ChanMap[MAX_EFFECT_CHANNELS];
    /* Wet buffer configuration is ACN channel order with N3D scaling:
     * * Channel 0 is the unattenuated mono signal.
     * * Channel 1 is OpenAL -X * sqrt(3)
     * * Channel 2 is OpenAL Y * sqrt(3)
     * * Channel 3 is OpenAL -Z * sqrt(3)
     * Consequently, effects that only want to work with mono input can use
     * channel 0 by itself. Effects that want multichannel can process the
     * ambisonics signal and make a B-Format source pan for first-order device
     * output (FOAOut).
     */
    alignas(16) ALfloat WetBuffer[MAX_EFFECT_CHANNELS][BUFFERSIZE];

    ALeffectslot() = default;
    ALeffectslot(const ALeffectslot&) = delete;
    ALeffectslot& operator=(const ALeffectslot&) = delete;
    ~ALeffectslot();

    DEF_NEWDEL(ALeffectslot)
};

ALenum InitEffectSlot(ALeffectslot *slot);
void UpdateEffectSlotProps(ALeffectslot *slot, ALCcontext *context);
void UpdateAllEffectSlotProps(ALCcontext *context);
ALvoid ReleaseALAuxiliaryEffectSlots(ALCcontext *Context);


EffectStateFactory *NullStateFactory_getFactory(void);
EffectStateFactory *ReverbStateFactory_getFactory(void);
EffectStateFactory *AutowahStateFactory_getFactory(void);
EffectStateFactory *ChorusStateFactory_getFactory(void);
EffectStateFactory *CompressorStateFactory_getFactory(void);
EffectStateFactory *DistortionStateFactory_getFactory(void);
EffectStateFactory *EchoStateFactory_getFactory(void);
EffectStateFactory *EqualizerStateFactory_getFactory(void);
EffectStateFactory *FlangerStateFactory_getFactory(void);
EffectStateFactory *FshifterStateFactory_getFactory(void);
EffectStateFactory *ModulatorStateFactory_getFactory(void);
EffectStateFactory *PshifterStateFactory_getFactory(void);

EffectStateFactory *DedicatedStateFactory_getFactory(void);


ALenum InitializeEffect(ALCcontext *Context, ALeffectslot *EffectSlot, ALeffect *effect);

void ALeffectState_DecRef(ALeffectState *state);

#endif
