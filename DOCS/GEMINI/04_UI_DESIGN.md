# CZ-101 EMULATOR - DISEÑO DE INTERFAZ

**Versión:** 1.0  
**Fecha:** 14 Diciembre 2025  
**Tema inicial:** Dark Mode

---

## 🎨 FILOSOFÍA DE DISEÑO

### Principios

1. **Fidelidad al Original**
   - Respetar el layout del CZ-101 (1984)
   - LCD 16×2 caracteres
   - Controles físicos simulados

2. **Modernidad Funcional**
   - Tema Dark Mode profesional
   - Animaciones suaves
   - Feedback visual inmediato

3. **Usabilidad**
   - Controles grandes y fáciles de usar
   - Tooltips informativos
   - Valores siempre visibles

---

## 🎨 TEMA: DARK MODE

### Paleta de Colores

```cpp
namespace Colors {
    // Backgrounds
    const juce::Colour bgPrimary   = juce::Colour(0xFF2A2A2A);  // Charcoal oscuro
    const juce::Colour bgSecondary = juce::Colour(0xFF1F1F1F);  // Más oscuro
    
    // Accents
    const juce::Colour accentPrimary   = juce::Colour(0xFF00BFFF);  // Neon azul
    const juce::Colour accentSecondary = juce::Colour(0xFF00FFFF);  // Cyan brillante
    
    // Text
    const juce::Colour textPrimary   = juce::Colour(0xFFFFFFFF);  // Blanco puro
    const juce::Colour textSecondary = juce::Colour(0xFFB0B0B0);  // Gris claro
    
    // UI Elements
    const juce::Colour border        = juce::Colour(0xFF404040);  // Gris medio
    const juce::Colour knobHighlight = juce::Colour(0xFF00BFFF);  // Azul
    const juce::Colour lcdBackground = juce::Colour(0xFF1A1A1A);  // Negro azulado
    const juce::Colour lcdText       = juce::Colour(0xFF00BFFF);  // Azul LCD
    
    // Effects
    const float glowAmount = 0.3f;  // Intensidad de glow
}
```

**Referencia:** `CZ101-DISENO-9-TEMAS.md` líneas 28-42

### Tipografía

```cpp
namespace Fonts {
    // LCD Display (monoespaciado)
    const juce::Font lcdFont = juce::Font("Courier New", 14.0f, juce::Font::bold);
    
    // Labels
    const juce::Font labelFont = juce::Font("Arial", 11.0f, juce::Font::plain);
    
    // Values
    const juce::Font valueFont = juce::Font("Arial", 14.0f, juce::Font::bold);
}
```

### Efectos Visuales

**Glow Effect (para elementos activos):**
```cpp
void drawGlow(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color) {
    juce::ColourGradient gradient(
        color.withAlpha(0.5f), bounds.getCentre(),
        color.withAlpha(0.0f), bounds.getBottomRight(),
        true
    );
    g.setGradientFill(gradient);
    g.fillEllipse(bounds.expanded(10.0f));
}
```

---

## 📐 LAYOUT PRINCIPAL

### Dimensiones del Plugin

**Tamaño total:** 800 × 600 pixels  
**Aspect ratio:** 4:3  
**Resizable:** No (fase inicial)

### Distribución de Áreas

```
┌─────────────────────────────────────────────────────────────┐
│                    HEADER (800×60)                          │
│  [Logo CZ-101]              [LCD 16×2]         [Preset]     │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│                   OSCILLATORS (800×180)                      │
│  ┌──────────────────────┐  ┌──────────────────────┐        │
│  │       DCO 1          │  │       DCO 2          │        │
│  │  [Wave] [Pitch]      │  │  [Wave] [Pitch]      │        │
│  │  [Detune] [Volume]   │  │  [Detune] [Volume]   │        │
│  └──────────────────────┘  └──────────────────────┘        │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                   ENVELOPES (800×180)                        │
│  ┌──────────────────────┐  ┌──────────────────────┐        │
│  │    DCW Envelope      │  │    DCA Envelope      │        │
│  │  [A] [D] [S] [R]     │  │  [A] [D] [S] [R]     │        │
│  └──────────────────────┘  └──────────────────────┘        │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                    EFFECTS (800×120)                         │
│  [Reverb]  [Chorus]  [Delay]                                │
│                                                              │
├─────────────────────────────────────────────────────────────┤
│                    FOOTER (800×60)                           │
│  [CPU: 2.3%]  [Voices: 3/8]  [Version 1.0.0]                │
└─────────────────────────────────────────────────────────────┘
```

---

## 🖼️ COMPONENTES UI

### 1. LCD Display (16×2)

**Dimensiones:** 280 × 60 pixels  
**Posición:** Centro superior

**Características:**
- Fondo negro azulado (#1A1A1A)
- Texto azul LCD (#00BFFF)
- Font monoespaciado (Courier New)
- 16 caracteres × 2 líneas
- Glow sutil alrededor

**Contenido típico:**
```
Línea 1: "Classic Lead    "  (nombre preset)
Línea 2: "Bank A  Patch 01"  (banco y número)
```

**Implementación:**
```cpp
class LCDDisplay : public juce::Component {
public:
    void setText(const juce::String& line1, const juce::String& line2);
    void paint(juce::Graphics& g) override;
    
private:
    juce::String m_line1;
    juce::String m_line2;
    static constexpr int CHARS_PER_LINE = 16;
    static constexpr int NUM_LINES = 2;
};
```

### 2. Knob Rotatorio

**Dimensiones:** 60 × 80 pixels (knob + label + value)  
**Rotación:** 270° (135° a cada lado del centro)

**Características:**
- Círculo con indicador de posición
- Label arriba (nombre del parámetro)
- Value abajo (valor numérico)
- Glow azul cuando se mueve
- Respuesta suave al mouse

**Estados:**
- Normal: Gris con borde
- Hover: Glow sutil
- Dragging: Glow intenso

**Implementación:**
```cpp
class Knob : public juce::Slider {
public:
    Knob(const juce::String& parameterName);
    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    
private:
    juce::String m_parameterName;
    bool m_isHovered = false;
    
    void drawRotarySlider(juce::Graphics& g, 
                         int x, int y, int width, int height,
                         float sliderPos,
                         float rotaryStartAngle,
                         float rotaryEndAngle);
};
```

### 3. Selector de Waveform

**Dimensiones:** 120 × 40 pixels  
**Tipo:** ComboBox estilizado

**Opciones:**
1. Sine
2. Sawtooth
3. Square
4. Triangle
5. Pulse
6. DoubleSine
7. HalfSine
8. ResonantSaw
9. ResonantTriangle
10. Trapezoid

**Características:**
- Dropdown con preview visual de waveform
- Texto + mini gráfico de onda
- Color azul cuando está abierto

### 4. Botón de Preset

**Dimensiones:** 100 × 30 pixels

**Tipos:**
- **Load:** Cargar preset
- **Save:** Guardar preset
- **Previous/Next:** Navegar presets

**Estados:**
- Normal: Fondo oscuro, texto blanco
- Hover: Borde azul
- Pressed: Fondo azul

---

## 🎛️ SECCIONES DETALLADAS

### Sección: Oscillators

**Layout:**
```
┌────────────────────────────────────────┐
│            OSCILLATOR 1                │
├────────────────────────────────────────┤
│  Waveform: [Dropdown ▼]               │
│                                        │
│  [Pitch]  [Detune]  [Volume]  [Sync]  │
│   ±48      ±100      0-100     ON/OFF │
│   semi     cents      %               │
└────────────────────────────────────────┘
```

**Parámetros:**
- **Waveform:** Dropdown (10 opciones)
- **Pitch:** Knob (-48 a +48 semitonos)
- **Detune:** Knob (-100 a +100 cents)
- **Volume:** Knob (0-100%)
- **Sync:** Toggle button (solo DCO2)

### Sección: Envelopes

**Layout:**
```
┌────────────────────────────────────────┐
│          DCW ENVELOPE                  │
├────────────────────────────────────────┤
│  [Attack] [Decay] [Sustain] [Release] │
│   0-2000  0-3000   0-100%    0-3000   │
│     ms      ms                  ms     │
│                                        │
│  [Gráfico visual del envelope]        │
└────────────────────────────────────────┘
```

**Características especiales:**
- Gráfico visual que muestra la curva del envelope
- Se actualiza en tiempo real al mover knobs
- Indicador de fase actual (Attack/Decay/Sustain/Release)

### Sección: Effects

**Layout:**
```
┌────────────────────────────────────────┐
│              EFFECTS                   │
├────────────────────────────────────────┤
│  REVERB        CHORUS         DELAY    │
│  [Size] [Mix]  [Rate] [Depth] [Time]  │
│                                [Fdbk]  │
│                                [Mix]   │
└────────────────────────────────────────┘
```

---

## 🎨 LOOK AND FEEL IMPLEMENTATION

### CZ101LookAndFeel Class

```cpp
class CZ101LookAndFeel : public juce::LookAndFeel_V4 {
public:
    CZ101LookAndFeel() {
        // Configurar colores base
        setColour(juce::ResizableWindow::backgroundColourId, Colors::bgPrimary);
        setColour(juce::Slider::thumbColourId, Colors::accentPrimary);
        setColour(juce::Slider::trackColourId, Colors::border);
        setColour(juce::TextButton::buttonColourId, Colors::bgSecondary);
        setColour(juce::TextButton::textColourOffId, Colors::textPrimary);
    }
    
    // Override métodos de dibujo
    void drawRotarySlider(juce::Graphics& g, 
                         int x, int y, int width, int height,
                         float sliderPos,
                         float rotaryStartAngle,
                         float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    void drawButtonBackground(juce::Graphics& g,
                             juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;
    
    void drawComboBox(juce::Graphics& g,
                     int width, int height,
                     bool isButtonDown,
                     int buttonX, int buttonY,
                     int buttonW, int buttonH,
                     juce::ComboBox& box) override;
};
```

**Referencia:** `CZ101-TEMAS-COMPLETADOS.md` líneas 124-262

---

## 🎬 ANIMACIONES

### Transiciones Suaves

**Knob rotation:**
```cpp
// Usar juce::AnimatedPosition para suavizar movimientos
class AnimatedKnob : public Knob {
private:
    juce::AnimatedPosition<float> m_animatedValue;
    
    void setValue(float newValue) {
        m_animatedValue.setValue(newValue, 0.2f);  // 200ms transition
    }
};
```

**Glow effect:**
```cpp
// Fade in/out del glow al hacer hover
class GlowComponent : public juce::Component {
private:
    float m_glowAlpha = 0.0f;
    
    void mouseEnter(const juce::MouseEvent&) override {
        juce::Desktop::getInstance().getAnimator()
            .animateComponent(this, getBounds(), m_glowAlpha, 1.0f, 200, false, 1.0, 0.0);
    }
};
```

### Feedback Visual

**Parameter change:**
- Knob: Glow azul durante 500ms después de cambio
- Value: Flash blanco durante 200ms
- LCD: Update inmediato sin parpadeo

---

## 📱 RESPONSIVE DESIGN (Futuro)

### Tamaños Soportados

**Fase 1 (actual):**
- Fixed: 800 × 600 pixels

**Fase 2 (futuro):**
- Small: 600 × 450 pixels
- Medium: 800 × 600 pixels
- Large: 1200 × 900 pixels

### Adaptaciones

**Small:**
- Knobs más pequeños (40px)
- LCD 12 caracteres
- Menos spacing

**Large:**
- Knobs más grandes (80px)
- Gráficos de envelope más detallados
- Más información visible

---

## 🧪 TESTING UI

### Checklist Visual

- [ ] Todos los knobs responden suavemente
- [ ] LCD muestra texto correctamente
- [ ] Colores consistentes con tema
- [ ] Glow effects funcionan
- [ ] Tooltips aparecen en hover
- [ ] Valores se actualizan en tiempo real
- [ ] No hay flickering
- [ ] Performance >60 FPS

### Herramientas

- **JUCE Component Debugger:** Ver jerarquía de componentes
- **Graphics Performance Monitor:** Medir FPS
- **Color Picker:** Verificar colores exactos

---

## 📚 RECURSOS

### Fonts

**Incluir en Resources/Fonts/:**
- `CourierNew.ttf` - Para LCD
- `Arial.ttf` - Para labels (opcional, usar system font)

### Images

**Incluir en Resources/Images/:**
- `logo_cz101.png` - Logo para header
- `waveform_previews/` - Previews de waveforms (opcional)

### Mockup de Referencia

**URL:** https://user-gen-media-assets.s3.amazonaws.com/seedream_images/d7d7f048-f70a-4733-9a68-9d6e30d46b29.png

**Referencia:** `CZ101-TEMAS-COMPLETADOS.md` línea 14

---

## 🎯 PRÓXIMOS PASOS

1. Implementar `CZ101LookAndFeel.h/cpp`
2. Crear componentes básicos (LCD, Knob, Button)
3. Construir layout principal en `PluginEditor`
4. Conectar UI a parámetros
5. Añadir animaciones y polish

---

**Última actualización:** 14 Diciembre 2025  
**Próximo documento:** `05_TESTING.md`
