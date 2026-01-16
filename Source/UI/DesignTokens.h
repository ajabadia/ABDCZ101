#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace CZ101 {
namespace UI {

namespace DesignTokens {

    static inline float layoutScale = 1.0f;
    
    namespace Colors {
        // --- Authentic CZ-101 Hardware Palette ---
        static inline const juce::Colour panelBackground    = juce::Colour(0xff3a3a3a); // Gris oscuro panel
        static inline const juce::Colour sectionBackground  = juce::Colour(0xff2a2a2a); // Gris más profundo zonas
        static inline const juce::Colour lcdBackground      = juce::Colour(0xffd2e4c8); // Verde pálido LCD
        static inline const juce::Colour lcdText            = juce::Colour(0xff1a1a1a); // Negro LCD
        
        static inline const juce::Colour czRed              = juce::Colour(0xffe94b35); // Acento rojo/naranja
        static inline const juce::Colour czOrange           = juce::Colour(0xfff5a623); // Acento naranja
        static inline const juce::Colour czBlue             = juce::Colour(0xff4a9eff); // Acento azul (secciones)
        static inline const juce::Colour czGreen            = juce::Colour(0xff50e3c2); // Acento verde (System)

        enum class VisualEffect {
            None,
            Scanlines,
            Glass
        };

        struct Palette {
            juce::Colour background;
            juce::Colour surface;
            juce::Colour accentCyan;
            juce::Colour accentOrange;
            juce::Colour accentTertiary;
            juce::Colour border;
            juce::Colour textPrimary;
            juce::Colour textSecondary;
            juce::Colour lcdBg;
            juce::Colour lcdText;
            juce::Colour glowColor;
            juce::Colour sectionBackground; 
            juce::Colour surfaceLight;
            juce::Colour modernIndicator;
            VisualEffect effect;
        };

        // 1. Dark (Default)
        static const Palette Dark = { 
            panelBackground, sectionBackground, czBlue, czOrange, czGreen,
            juce::Colour(0xff202020), juce::Colours::white, juce::Colours::lightgrey,
            lcdBackground, lcdText, juce::Colours::transparentBlack,
            juce::Colour(0xff2a2a2a), juce::Colour(0xff4a4a4a), 
            juce::Colours::orange.withAlpha(0.15f), VisualEffect::None
        };

        // 2. Light
        static const Palette Light = { 
            juce::Colour(0xfff5f5f7), juce::Colour(0xffffffff), 
            czBlue.withMultipliedSaturation(0.8f), czOrange, czGreen.withMultipliedSaturation(0.8f),
            juce::Colour(0xffd1d1d6), juce::Colours::black, juce::Colour(0xff3a3a3c),
            lcdBackground.brighter(0.1f), lcdText, juce::Colours::transparentBlack,
            juce::Colour(0xffe5e5ea), juce::Colour(0xfff0f0f2), 
            juce::Colours::blue.withAlpha(0.1f), VisualEffect::None
        };

        // 3. Vintage
        static const Palette Vintage = { 
            juce::Colour(0xff8c8c8c), juce::Colour(0xff666666), 
            juce::Colour(0xff1dbb9b), juce::Colour(0xffd35400), juce::Colour(0xff2980b9),
            juce::Colour(0xff4d4d4d), juce::Colours::white, juce::Colour(0xffdddddd),
            juce::Colour(0xff66bb6a), juce::Colours::black, juce::Colours::transparentBlack,
            juce::Colour(0xff555555), juce::Colour(0xff777777), 
            juce::Colours::gold.withAlpha(0.2f), VisualEffect::None
        };

        // 4. Retro Beige
        static const Palette RetroBeige = { 
            juce::Colour(0xffd2ccb2), juce::Colour(0xffe1dcc5), 
            juce::Colour(0xff008080), juce::Colour(0xffb35c00), juce::Colour(0xff5d5d5d),
            juce::Colour(0xffb2a78d), juce::Colour(0xff2c2c2c), juce::Colour(0xff5a5a5a),
            juce::Colour(0xff2a1b00), juce::Colour(0xffffb000), juce::Colour(0xffffb000), // Amber LCD
            juce::Colour(0xffbab291), juce::Colour(0xffece9d8), 
            juce::Colours::white.withAlpha(0.15f), VisualEffect::Scanlines
        };

        // 5. CyberGlow
        static const Palette CyberGlow = { 
            juce::Colour(0xff0b0e14), juce::Colour(0xff161b22), 
            juce::Colour(0xff00f2ff), juce::Colour(0xffff007a), juce::Colour(0xffbc13fe),
            juce::Colour(0xff30363d), juce::Colour(0xffc9d1d9), juce::Colour(0xff8b949e),
            juce::Colour(0xff000000), juce::Colour(0xff00f2ff), juce::Colour(0xff00f2ff),
            juce::Colour(0xff0d1117), juce::Colour(0xff21262d), 
            juce::Colour(0xffff007a).withAlpha(0.2f), VisualEffect::None
        };

        // 6. Neon Retro
        static const Palette NeonRetro = { 
            juce::Colour(0xff120458), juce::Colour(0xff2d025d), 
            juce::Colour(0xffff00c8), juce::Colour(0xff39ff14), juce::Colour(0xff7b00ff),
            juce::Colour(0xffff00c8), juce::Colours::white, juce::Colour(0xfff0f0f0),
            juce::Colour(0xff000000), juce::Colour(0xffff00c8), juce::Colour(0xffff00c8),
            juce::Colour(0xff1b0044), juce::Colour(0xff40058b), 
            juce::Colour(0xff39ff14).withAlpha(0.2f), VisualEffect::Scanlines
        };

        // 7. Steampunk
        static const Palette Steampunk = { 
            juce::Colour(0xff3e2723), juce::Colour(0xff4e342e), 
            juce::Colour(0xffcd7f32), juce::Colour(0xffb87333), juce::Colour(0xff8b4513),
            juce::Colour(0xff211a17), juce::Colour(0xffd7ccc8), juce::Colour(0xffa1887f),
            juce::Colour(0xff263238), juce::Colour(0xffffab40), juce::Colour(0xffffab40),
            juce::Colour(0xff321a11), juce::Colour(0xff5d4037), 
            juce::Colour(0xffcd7f32).withAlpha(0.25f), VisualEffect::None
        };

        // 8. Apple Silicon
        static const Palette AppleSilicon = { 
            juce::Colour(0xfff0f0f0), juce::Colour(0xffffffff), 
            juce::Colour(0xff007aff), juce::Colour(0xffff9500), juce::Colour(0xff5856d6),
            juce::Colour(0xffd1d1d6), juce::Colours::black, juce::Colour(0xff3a3a3c),
            juce::Colour(0xffffffff), juce::Colours::black, juce::Colours::transparentBlack,
            juce::Colour(0xffe5e5ea), juce::Colour(0xfffafafa), 
            juce::Colours::grey.withAlpha(0.1f), VisualEffect::Glass
        };

        // 9. Retro Terminal
        static const Palette RetroTerminal = { 
            juce::Colour(0xff000000), juce::Colour(0xff0c0c0c), 
            juce::Colour(0xff00ff41), juce::Colour(0xff008f11), juce::Colour(0xff003b00),
            juce::Colour(0xff00ff41), juce::Colour(0xff00ff41), juce::Colour(0xff00ff41),
            juce::Colour(0xff000000), juce::Colour(0xff00ff41), juce::Colour(0xff00ff41),
            juce::Colour(0xff050505), juce::Colour(0xff121212), 
            juce::Colours::white.withAlpha(0.2f), VisualEffect::Scanlines
        };

        static inline const Palette& getCurrentPalette(int themeIndex) {
            switch (themeIndex) {
                case 1:  return Light;
                case 2:  return Vintage;
                case 3:  return RetroBeige;
                case 4:  return CyberGlow;
                case 5:  return NeonRetro;
                case 6:  return Steampunk;
                case 7:  return AppleSilicon;
                case 8:  return RetroTerminal;
                default: return Dark;
            }
        }
    }

    namespace Metrics {
        static inline const float cornerRadiusLarge      = 8.0f;
        static inline const float cornerRadiusSmall      = 2.0f;
    }

    namespace Typography {
        static inline float getHeaderSize()    { return 18.0f * layoutScale; }
        static inline float getSubHeaderSize() { return 16.0f * layoutScale; }
        static inline float getBaseSize()      { return 13.0f * layoutScale; }
        static inline float getTinySize()      { return 10.0f * layoutScale; }
    }

} // namespace DesignTokens

} // namespace UI
} // namespace CZ101
