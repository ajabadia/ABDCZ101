// ================================================================================================
// Fase 1: Integración wire↔domain para interfaz ABDCZ101
// Conecta los parámetros traducidos con el procesador de síntesis
// ================================================================================================

#include "DspCz1.h"
#include "DspCz1VoiceState.h"

// ================================================================================================
// Estado global de parámetros CZ-101 (para interfaz y procesamiento en tiempo real)
// ================================================================================================

// Valores físicos actuales (lo que ve/edita el usuario)
static float g_cutoffHz = 2000.0f;       // Cutoff frequency en Hz
static float g_resonance = 0.5f;         // Resonance 0.0-1.0
static float g_lfoRateHz = 1.0f;         // LFO rate en Hz
static float g_lfoAmount = 0.5f;         // LFO amount 0.0-1.0
static float g_fineTuneCents = 0.0f;     // Fine tune en cents
static float g_pitchBendSt = 0.0f;       // Pitch bend en semitonos

// Valores CC/MIDI actuales (lo que llega por hardware)
static uint16_t g_lastCutoffCC = 0;      // Último valor CC recibido
static uint16_t g_lastResonanceCC = 0;
static uint16_t g_lastLfoRateNRPN = 0;
static uint16_t g_lastLfoAmountCC = 0;
static uint16_t g_lastFineTuneRPN = 0;
static uint16_t g_lastPitchBend = 0;

// ================================================================================================
// Función: processControlChange
// Desprocesa cambios de control MIDI CC/NRPN usando wireToDomain
// ================================================================================================

void processControlChange(int ccNumber, uint16_t value) {
    using namespace abd::dsp;
    
    switch (ccNumber) {
        case 74: // CC#74 - Filter Cutoff (también LFO Amount)
            // Si el sistema sabe a qué parámetro afecta, usa el correspondiente
            // Por ahora asumenos Cutoff cuando viene en el contexto de filtros
            g_cutoffHz = wireToDomain(Cz101ParameterMeta::Cutoff{}, value);
            g_lastCutoffCC = value;
            // Aquí se aplicaría: filter.setCutoff(g_cutoffHz);
            break;
            
        case 71: // CC#71 - Resonance
            g_resonance = wireToDomain(Cz101ParameterMeta::Resonance{}, value);
            g_lastResonanceCC = value;
            // Aplicaríamos: oscillator.setResonance(g_resonance);
            break;
            
        case 6:  // NRPN #0 - LFO Rate
            g_lfoRateHz = wireToDomain(Cz101ParameterMeta::LFO1_Rate{}, value);
            g_lastLfoRateNRPN = value;
            // Aplicaríamos: lfo1.setRate(g_lfoRateHz);
            break;
            
        default:
            // Control no mapeado explícitamente
            break;
    }
}

// ================================================================================================
// Función: processPitchBend
// Maneja el pitch bend channel usando wireToDomain/PitchBend
// ================================================================================================
void processPitchBend(uint16_t lsb, uint16_t msb) {
    using namespace abd::dsp;
    
    // Combinamos LSB y MSB: valor 0-16383
    int combinedValue = (static_cast<int>(msb) << 7) | static_cast<int>(lsb);
    
    // Normalizamos a rango -12 a +12 semitonos
    float normalized = (static_cast<float>(combinedValue) / 16383.0f) * 24.0f - 12.0f;
    
    // Usamos la función wireToDomain con parámetro PitchBend
    g_pitchBendSt = wireToDomain(Cz101ParameterMeta::PitchBend{}, 
                                  normalized);
    
    g_lastPitchBend = combinedValue;
    // Aplicaríamos el pitch bend a los oscilladores
}

// ================================================================================================
// Funciones de acceso para la interfaz de usuario
// ================================================================================================

// Devuelven los valores físicos actuales (lo que muestra la UI)
float getCutoffHz() { return g_cutoffHz; }
float getResonance() { return g_resonance; }
float getLFORateHz() { return g_lfoRateHz; }
float getLFOAmount() { return g_lfoAmount; }
float getFineTuneCents() { return g_fineTuneCents; }
float getPitchBendSt() { return g_pitchBendSt; }

// Convierte un valor físico a CC para mostrar/editar en la UI
uint16_t ccFromCutoffHz(float hz) { 
    return domainToWire(Cz101ParameterMeta::Cutoff{}, hz); 
}
uint16_t ccFromResonance(float val) { 
    return domainToWire(Cz101ParameterMeta::Resonance{}, val); 
}
uint16_t ccFromLFORateHz(float hz) { 
    return domainToWire(Cz101ParameterMeta::LFO1_Rate{}, hz); 
}
uint16_t ccFromLFOAmount(float val) { 
    return domainToWire(Cz101ParameterMeta::LFO1_Amount{}, val); 
}

// Convierte un valor CC/MIDI actual a valor físico para mostrar en la UI
float getCutoffFromCC(uint16_t ccValue) {
    return wireToDomain(Cz101ParameterMeta::Cutoff{}, ccValue);
}
float getResonanceFromCC(uint16_t ccValue) {
    return wireToDomain(Cz101ParameterMeta::Resonance{}, ccValue);
}
float getLFORateFromCC(uint16_t ccValue) {
    return wireToDomain(Cz101ParameterMeta::LFO1_Rate{}, ccValue);
}
float getLFOAmountFromCC(uint16_t ccValue) {
    return wireToDomain(Cz101ParameterMeta::LFO1_Amount{}, ccValue);
}

// ================================================================================================
// Función: applyParametersToVoice
// Aplica los parámetros actuales a una voz CZ-1 activa
// ================================================================================================
void applyParametersToVoice(DspCz1VoiceState& voice) {
    using namespace abd::dsp;
    
    // Aplicar cutoff al filtro de la voz
    // voice.setCutoff(g_cutoffHz); // Cuando esté implementado
    
    // Aplicar resonance
    // voice.setResonance(g_resonance);
    
    // Aplicar fine tune
    // voice.setDcoFineTune(g_fineTuneCents);
    
    // El resto se aplicaría según la arquitectura específica
}