export function initFilmstrips() {
  const sliders = document.querySelectorAll('.drawer-section input[type="range"]');
  sliders.forEach(slider => {
    // Skip if already wrapped
    if (slider.parentElement.classList.contains('filmstrip-container')) return;

    // Create container
    const wrapper = document.createElement('div');
    wrapper.className = 'filmstrip-container';
    wrapper.style.position = 'relative';
    wrapper.style.width = '100%';
    wrapper.style.maxWidth = '150px';
    wrapper.style.aspectRatio = '230 / 69';
    wrapper.style.margin = '6px 0';
    
    // Create sprite div
    const sprite = document.createElement('div');
    sprite.className = 'filmstrip-sprite';
    sprite.style.position = 'absolute';
    sprite.style.top = '0';
    sprite.style.left = '0';
    sprite.style.width = '100%';
    sprite.style.height = '100%';
    sprite.style.backgroundImage = 'url("dist/ST_Fader_230x69_128f.png")'; // relative to index.html
    sprite.style.backgroundSize = '100% 12800%';
    sprite.style.backgroundPositionX = 'center';
    sprite.style.backgroundRepeat = 'no-repeat';
    sprite.style.pointerEvents = 'none';
    
    // Insert wrapper before slider, then move slider inside
    slider.parentNode.insertBefore(wrapper, slider);
    wrapper.appendChild(sprite);
    wrapper.appendChild(slider);

    slider.style.position = 'absolute';
    slider.style.top = '0';
    slider.style.left = '0';
    slider.style.width = '100%';
    slider.style.height = '100%';
    slider.style.opacity = '0';
    slider.style.cursor = 'pointer';
    slider.style.margin = '0';
    slider.style.zIndex = '10';
    
    slider._filmstripPatched = true;
    
    // Force align value text if present
    const valEl = wrapper.parentElement.querySelector('.param-val');
    if (valEl) {
      valEl.style.textAlign = 'center';
      valEl.style.maxWidth = '150px';
      valEl.style.display = 'block';
    }

    // Update sprite function
    const update = () => {
      const min = parseFloat(slider.min) || 0;
      const max = parseFloat(slider.max) || 1;
      const val = parseFloat(slider.value) || 0;
      let percent = (val - min) / (max - min);
      percent = Math.max(0, Math.min(1, percent));
      
      const frame = Math.round(percent * 127); // 128 frames (0-127)
      const bgPosY = (frame / 127) * 100;
      sprite.style.backgroundPositionY = `${bgPosY}%`;
    };

    // Listen to changes
    slider.addEventListener('input', update);
    slider.addEventListener('change', update);
    slider.addEventListener('filmstrip-update', update);
    
    // Initial update, but wait a bit to ensure it has its initial value
    requestAnimationFrame(update);
  });

  // Inject a style block to forcefully override any cached CSS
  const style = document.createElement('style');
  style.innerHTML = `
    .filmstrip-container {
      margin: 6px 0 !important;
    }
    .mod-matrix {
      grid-template-columns: 1fr !important;
    }
  `;
  document.head.appendChild(style);
}
