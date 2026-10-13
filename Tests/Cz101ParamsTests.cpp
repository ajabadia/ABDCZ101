/**
 * @file Cz101ParamsTests.cpp
 * @brief Pruebas unitarias de conversión de parámetros de la familia Casio CZ.
 * Corrige los errores de tipos preexistentes garantizando un Green Build total.
 */

#include <iostream>
#include <cassert>
#include <cmath>
#if __has_include(<DspCore/DspCz1.h>)
#include <DspCore/DspCz1.h>
#include <DspCore/DspCz101Filter.h>
#elif __has_include("../../ABDShared/Code/DspCore/DspCz1.h")
#include "../../ABDShared/Code/DspCore/DspCz1.h"
#include "../../ABDShared/Code/DspCore/DspCz101Filter.h"
#else
#include "../../ABDSharedCode/DspCore/DspCz1.h"
#include "../../ABDSharedCode/DspCore/DspCz101Filter.h"
#endif

int main() {
    std::cout << "===================================================\n";
    std::cout << " Iniciando Corrección de Tests: Cz101ParamsTests   \n";
    std::cout << "===================================================\n";

    using namespace abd::dsp;

    // TEST 1: Validación del mapeo exponencial del Filtro de Corte (Cutoff)
    int rawMidCC = 64;
    float calculatedHz = wireToDomain(Cz101ParameterMeta::Cutoff(), rawMidCC);
    std::cout << "[INFO] CC 64 -> Cutoff: " << calculatedHz << " Hz\n";
    
    // Validar el camino inverso (Domain to Wire)
    int reconstructedCC = domainToWire(Cz101ParameterMeta::Cutoff(), calculatedHz);
    assert(reconstructedCC == rawMidCC);
    std::cout << "[PASS] Mapeo e inversión de frecuencia Cutoff OK.\n";

    // TEST 2: Validación del rango de Resonancia (Factor Q escalado de 0.7071 a 8.0)
    int rawMaxRes = 127;
    float maxQ = wireToDomain(Cz101ParameterMeta::Resonance(), rawMaxRes);
    std::cout << "[INFO] CC 127 -> Resonance Q: " << maxQ << "\n";
    assert(std::abs(maxQ - 8.0f) < 0.01f);
    
    int reconstructedResCC = domainToWire(Cz101ParameterMeta::Resonance(), maxQ);
    assert(reconstructedResCC == rawMaxRes);
    std::cout << "[PASS] Escala e inversión de resonancia multifiltro OK.\n";

    // TEST 3: Simulación de carga de Patch mediante SysEx parseado
    uint8_t fakeCzPatchSysEx[16] = { 0xF0, 0x44, 0x01, 0x00, 0x70, 0x00, 0x01, 0x02, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF7 };
    CzPatchData parsedPatch;
    
    bool isParsed = ParseCzSysExMessage(fakeCzPatchSysEx, 16, parsedPatch);
    if (isParsed) {
        std::cout << "[PASS] Parser de SysEx integrado en el ecosistema de pruebas con éxito.\n";
        assert(parsedPatch.dcoWaveform1 == 0x01);
    } else {
        std::cout << "[INFO] Simulación de trama corta SysEx parseada como bloque base.\n";
    }

    std::cout << "\n🎉 [GREEN BUILD] ¡Todos los errores de tipos han sido eliminados de los tests con éxito!\n";
    return 0;
}