// SysExRoundTripTest.cpp — Regression test for SysEx encode/decode.
// Covers createPatchDump → decodePatch/decodeOnePatch/handleSysEx for
// single patches, CZ-1 names, extended modulation, and bank dumps.

#include "../MIDI/SysExManager.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>

using namespace CZ101::State;
using CZ101::MIDI::SysExManager;

static int gFailures = 0;

static void fail(const std::string& what) { std::cerr << "  FAIL: " << what << "\n"; ++gFailures; }
static void expectClose(float o, float d, float t, const std::string& w) { if (std::abs(o-d)>t) fail(w+" o="+std::to_string(o)+" d="+std::to_string(d)); }
static void expectInt(int o, int d, const std::string& w) { if (o!=d) fail(w+" o="+std::to_string(o)+" d="+std::to_string(d)); }

static void setEnv(EnvelopeData& e, int sus, int end, const float* r, const float* l) {
    for (int i=0;i<8;++i) { e.rates[i]=r[i]; e.levels[i]=l[i]; } e.sustainPoint=sus; e.endPoint=end;
}

static Preset makeFullPreset(const std::string& name) {
    Preset p(name); p.author="Test";
    p.parameters["LINE_SELECT"]=2;p.parameters["OCTAVE"]=0;p.parameters["OSC2_DETUNE"]=1.5f;
    p.parameters["LFO_WAVE"]=2;p.parameters["LFO_DELAY"]=0.5f;p.parameters["LFO_RATE"]=12;p.parameters["LFO_DEPTH"]=0.7f;
    p.parameters["OSC1_WAVEFORM"]=4;p.parameters["OSC1_WAVEFORM2"]=3;p.parameters["OSC1_WINDOW"]=2;
    p.parameters["OSC2_WAVEFORM"]=1;p.parameters["OSC2_WAVEFORM2"]=0;p.parameters["OSC2_WINDOW"]=5;
    p.parameters["LINE_MODULATION"]=3;p.parameters["MOD_SPECIAL"]=0;
    // KF: CZ-101 SysEx only has DCA and DCW key follow per line (no pitch KF)
    p.parameters["LINE1_KF_DCW"]=2;p.parameters["LINE1_KF_DCA"]=9;
    p.parameters["LINE2_KF_DCW"]=3;p.parameters["LINE2_KF_DCA"]=5;
    p.parameters["LINE1_VELO_DCA"]=10;p.parameters["LINE1_VELO_DCW"]=5;p.parameters["LINE1_VELO_PITCH"]=0;
    p.parameters["LINE2_VELO_DCA"]=12;p.parameters["LINE2_VELO_DCW"]=4;p.parameters["LINE2_VELO_PITCH"]=15;
    const float r[8]={0.9f,0.5f,0.3f,0.8f,0.4f,0.6f,0.7f,0.2f};
    const float l[8]={1,0.6f,0.8f,0,0.3f,0.5f,0.9f,0.1f};
    const float pl[8]={1,0.6f,0.8f,0.5f,0.3f,0.9f,0.7f,0.2f};
    setEnv(p.dcaEnv,2,3,r,l);setEnv(p.dcwEnv,1,4,r,l);setEnv(p.pitchEnv,2,3,r,pl);
    setEnv(p.dcaEnv2,0,1,r,l);setEnv(p.dcwEnv2,3,5,r,l);setEnv(p.pitchEnv2,1,2,r,pl);
    return p;
}

static void checkParams(const Preset& a, const Preset& b) {
    for (auto& [k,v]:a.parameters) { auto it=b.parameters.find(k); if (it==b.parameters.end()) fail("missing "+k); else { float t=k.find("LFO_RATE")!=std::string::npos?0.25f:0.05f; expectClose(v,it->second,t,"param "+k); } }
}
static void checkEnv(const EnvelopeData& a, const EnvelopeData& b, const std::string& n) {
    for (int i=0;i<8;++i) { expectClose(a.rates[i],b.rates[i],0.02f,n+" r["+std::to_string(i)+"]"); expectClose(a.levels[i],b.levels[i],0.02f,n+" l["+std::to_string(i)+"]"); }
    expectInt(a.sustainPoint,b.sustainPoint,n+" sus"); expectInt(a.endPoint,b.endPoint,n+" end");
}

// 1. Full preset single-patch
static int tFullPreset() {
    std::cout<<"--- Full preset ---\n"; Preset o=makeFullPreset("RT"); SysExManager m;
    auto d=m.createPatchDump(o); Preset dec;
    if (!SysExManager::decodePatch((const uint8_t*)d.getData(),dec)) { fail("decodePatch false"); return 1; }
    checkParams(o,dec); checkEnv(o.dcaEnv,dec.dcaEnv,"dca1"); checkEnv(o.dcwEnv,dec.dcwEnv,"dcw1");
    checkEnv(o.pitchEnv,dec.pitchEnv,"pt1"); checkEnv(o.dcaEnv2,dec.dcaEnv2,"dca2"); checkEnv(o.dcwEnv2,dec.dcwEnv2,"dcw2"); checkEnv(o.pitchEnv2,dec.pitchEnv2,"pt2");
    std::cout<<"  full preset: "<<(gFailures==0?"OK":"FAILURES")<<"\n"; return gFailures;
}

// 2. Extended modulation
static int tMod() {
    std::cout<<"--- Extended modulation ---\n"; int l=0;
    for (int mode=0;mode<=5;++mode) for (bool sp:{false,true}) { Preset p("M"); p.parameters["LINE_MODULATION"]=(float)mode; p.parameters["MOD_SPECIAL"]=sp?1:0; SysExManager m; auto d=m.createPatchDump(p); Preset dec; SysExManager::decodePatch((const uint8_t*)d.getData(),dec); if ((int)std::lround(dec.parameters["LINE_MODULATION"])!=mode){fail("mode"+std::to_string(mode));++l;} if ((dec.parameters["MOD_SPECIAL"]>0.5f)!=sp){fail("spe"+std::to_string(sp));++l;} }
    gFailures+=l; std::cout<<"  mod: "<<(l==0?"OK":"FAILURES")<<"\n"; return l;
}

// 3. PDL detune sweep
static int tDetune() {
    std::cout<<"--- Detune sweep (PDL) ---\n"; int l=0; float v[]={0,0.05f,0.5f,1,1.5f,3,11.75f,-0.5f,-1.25f,-11.5f};
    for (float dt:v) { Preset p("D"); p.parameters["OSC2_DETUNE"]=dt; SysExManager m; auto d=m.createPatchDump(p); Preset dec; SysExManager::decodePatch((const uint8_t*)d.getData(),dec); if (std::abs(dt-dec.parameters["OSC2_DETUNE"])>0.02f) { fail("dt "+std::to_string(dt)); ++l; } else std::cout<<"  dt "<<dt<<" -> "<<dec.parameters["OSC2_DETUNE"]<<" OK\n"; }
    gFailures+=l; std::cout<<"  detune: "<<(l==0?"OK":"FAILURES")<<"\n"; return l;
}

// 4. CZ-1 name (opMode >= 2)
static int tCZ1Name() {
    std::cout<<"--- CZ-1 name ---\n"; int l=0;
    for (const char* nm:{"MoogBass","CZ1-16charName!@","Synth"}) { Preset p(nm); SysExManager m; auto d=m.createPatchDump(p,2); std::vector<Preset> v; SysExManager m2; m2.setProtectionState(false,true); m2.onPresetParsed=[&](auto& pr){v.push_back(pr);}; m2.handleSysEx(d.getData(),(int)d.getSize(),"");
        if (v.empty()) { fail("CZ-1 decode "+std::string(nm)); ++l; continue; }
        std::string exp(nm); if (exp.size()>16) exp=exp.substr(0,16); exp.erase(exp.find_last_not_of(" \n\r\t")+1);
        if (v[0].name!=exp) { fail("CZ-1 name '"+exp+"' vs '"+v[0].name+"'"); ++l; } else std::cout<<"  CZ-1 name '"<<nm<<"' -> '"<<v[0].name<<"' OK\n"; }
    gFailures+=l; std::cout<<"  CZ-1 name: "<<(l==0?"OK":"FAILURES")<<"\n"; return l;
}

// 5. Bank dump (multi-patch handleSysEx loop)
static int tBank() {
    std::cout<<"--- Bank dump ---\n"; int l=0; std::vector<Preset> orig; juce::MemoryBlock bank;
    for (int i=0;i<3;++i) { Preset p=makeFullPreset("BP"+std::to_string(i)); p.parameters["OSC2_DETUNE"]=i*0.5f; p.parameters["LINE_MODULATION"]=(float)i; orig.push_back(p); SysExManager m; auto d=m.createPatchDump(p); bank.append(d.getData(),d.getSize()); }
    std::vector<Preset> parsed; SysExManager m; m.setProtectionState(false,true); m.onPresetParsed=[&](auto& pr){parsed.push_back(pr);}; m.handleSysEx(bank.getData(),(int)bank.getSize(),"BK");
    if (parsed.size()!=orig.size()) { fail("count "+std::to_string(parsed.size())); ++l; } else for (size_t i=0;i<parsed.size();++i) { checkParams(orig[i],parsed[i]); checkEnv(orig[i].dcaEnv,parsed[i].dcaEnv,"bdca"+std::to_string(i)); }
    gFailures+=l; std::cout<<"  bank: "<<(l==0?"OK":"FAILURES")<<"\n"; return l;
}

int main() { std::cout<<"========================================\nCZ-101 SysEx Round-Trip Test\n========================================\n"; int f=0; f+=tFullPreset(); f+=tMod(); f+=tDetune(); f+=tCZ1Name(); f+=tBank(); if (f==0) { std::cout<<"ALL SYSEX ROUND-TRIP TESTS PASSED\n"; return 0; } std::cerr<<f<<" failure(s)\n"; return 1; }