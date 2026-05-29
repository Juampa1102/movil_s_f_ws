/* ══════════════════════════════════════════════════════════
   MUSEO ROBOT — Lógica principal
   Plataforma Robótica Educativa Modular
   ══════════════════════════════════════════════════════════ */

'use strict';

/* ─── Configuración de archivos ──────────────────────────
   Detecta extensiones automáticamente: .jpg, .png, .webp
   ──────────────────────────────────────────────────────── */

/** Piezas de ensamblaje con nombre visible */
const PIECES = [
  { file: 'chasis',         label: 'Chasis' },
  { file: 'carcasa_futbol', label: 'Carcasa Fútbol' },
  { file: 'tapa_futbol',    label: 'Tapa Fútbol' },
  { file: 'carcasa_sumo',   label: 'Carcasa Sumo' },
  { file: 'tapa_sumo',      label: 'Tapa Sumo' },
  { file: 'tornillo_m2',    label: 'Tornillo M2' },
  { file: 'llantas',        label: 'Llantas' },
];

/** Componentes electrónicos con información detallada */
const COMPONENTS = [
  {
    id: 'esp32',
    name: 'ESP32 DevKit',
    file: 'esp32',
    badge: 'Microcontrolador',
    desc: 'El ESP32 es el cerebro del robot. Es un microcontrolador de doble núcleo de 32 bits con conectividad WiFi y Bluetooth integrada, desarrollado por Espressif Systems.',
    how: 'Ejecuta el código del robot: lee sensores, calcula la estrategia de movimiento y envía señales de control a los motores mediante PWM.',
    power: '3.3 V lógica interna; se alimenta con 5 V a través del pin VIN o por USB. Consumo típico: 80–240 mA según actividad.',
    role: 'Coordina todos los subsistemas. Recibe datos de los sensores ToF, decide la dirección y velocidad, y ordena al driver DRV8833 la potencia a cada motor.',
  },
  {
    id: 'motor_n20',
    name: 'Motor DC N20 con Encoder',
    file: 'motor_n20',
    badge: 'Actuador',
    desc: 'Motor de corriente continua de tamaño miniatura con caja reductora metálica y encoder magnético de cuadratura integrado. Ofrece alto torque en un formato compacto.',
    how: 'Convierte energía eléctrica en movimiento giratorio. El encoder mide la velocidad y posición exacta de giro enviando pulsos al microcontrolador.',
    power: '3–6 V DC. Corriente en vacío ≈ 30 mA; corriente de parada (stall) hasta 800 mA a 6 V.',
    role: 'Impulsa las ruedas del robot. El ESP32 ajusta la velocidad individualmente en cada motor para girar, avanzar o detenerse con precisión.',
  },
  {
    id: 'vl53l0x',
    name: 'Sensor ToF VL53L0X',
    file: 'vl53l0x',
    badge: 'Sensor',
    desc: 'Sensor de tiempo de vuelo (Time-of-Flight) de STMicroelectronics. Mide distancias con un láser infrarrojo invisible y un detector de fotones. Rango: 30–2000 mm.',
    how: 'Emite pulsos de luz infrarroja y mide el tiempo que tarda en rebotar contra un objeto. Con la velocidad de la luz calcula la distancia en milisegundos.',
    power: '2.6–3.5 V. Consumo: 19 mA en modo de medición continua.',
    role: 'Detecta la posición del adversario en sumo y la posición de la pelota en fútbol. El robot usa estos datos para reaccionar en tiempo real.',
  },
  {
    id: 'drv8833',
    name: 'Driver de Motores DRV8833',
    file: 'drv8833',
    badge: 'Driver',
    desc: 'Circuito integrado puente H doble de Texas Instruments. Controla dos motores DC de forma independiente, permitiendo rotación en ambos sentidos y frenado.',
    how: 'Recibe señales PWM de baja potencia del ESP32 y las amplifica para manejar la corriente que necesitan los motores, protegiendo el microcontrolador.',
    power: 'Alimentación de motor: 2.7–10.8 V. Corriente de salida: 1.5 A continua / 2 A pico por canal.',
    role: 'Intermediario de potencia entre el ESP32 y los motores N20. Sin él, el ESP32 se dañaría al intentar alimentar directamente los motores.',
  },
  {
    id: 'sg90',
    name: 'Servomotor SG90',
    file: 'sg90',
    badge: 'Actuador',
    desc: 'Servomotor de posición de bajo costo y tamaño pequeño. Incluye una caja reductora plástica y un potenciómetro interno para retroalimentación de posición. Rango: 0–180°.',
    how: 'El ESP32 envía una señal PWM con un pulso de 1–2 ms. El servo interpreta la duración del pulso como un ángulo y mueve su brazo a esa posición exacta.',
    power: '4.8–6 V. Corriente: 100–250 mA dependiendo de la carga. Torque: 1.8 kg·cm a 4.8 V.',
    role: 'Acciona mecanismos de la estructura: brazos, palas o sensores orientables. Permite al robot adoptar posiciones específicas durante la competencia.',
  },
  {
    id: 'tca9548a',
    name: 'Multiplexor I²C TCA9548A',
    file: 'tca9548a',
    badge: 'Comunicación',
    desc: 'Multiplexor/demultiplexor I²C de 8 canales de Texas Instruments. Permite conectar múltiples dispositivos con la misma dirección I²C en un solo bus sin conflictos.',
    how: 'El ESP32 le indica qué canal activar. Solo ese canal queda conectado al bus I²C, permitiendo comunicarse con el sensor deseado sin interferencias.',
    power: '1.65–5.5 V. Consumo muy bajo: 10 µA en espera.',
    role: 'Soluciona el problema de los sensores VL53L0X que todos comparten la misma dirección I²C (0x29). Permite usar hasta 8 sensores simultáneamente.',
  },
  {
    id: 'bateria18650',
    name: 'Batería 18650',
    file: 'bateria18650',
    badge: 'Energía',
    desc: 'Celda de ion litio de formato cilíndrico estándar (18 mm diámetro × 65 mm largo). Alta densidad de energía, recargable. Ampliamente usada en electrónica portátil.',
    how: 'Almacena energía química y la libera como corriente eléctrica mediante reacciones electroquímicas de intercalación de iones litio entre electrodos.',
    power: 'Tensión nominal: 3.7 V. Tensión máxima: 4.2 V. Capacidad típica: 2000–3500 mAh. Descarga máxima: 5–20 A según modelo.',
    role: 'Fuente de energía principal del robot. Dos celdas en serie ofrecen 7.4 V para los motores; reguladores buck convierten esta tensión para el ESP32 y sensores.',
  },
  {
    id: 'mp1584',
    name: 'Buck Converter MP1584',
    file: 'mp1584',
    badge: 'Regulación',
    desc: 'Regulador reductor de voltaje (buck converter) de alta eficiencia basado en el chip MP1584 de MPS. Convierte una tensión mayor a una menor con muy poca pérdida de calor.',
    how: 'Usa conmutación rápida (PWM) de un transistor MOSFET junto a un inductor y capacitor para reducir el voltaje de entrada de manera eficiente (>92%).',
    power: 'Entrada: 4.5–28 V. Salida: 0.8–20 V ajustable. Corriente de salida: hasta 3 A. Eficiencia: ~92%.',
    role: 'Convierte los 7.4 V de las baterías a 5 V para el ESP32 y a 3.3 V para sensores, asegurando voltajes estables sin sobrecalentar el circuito.',
  },
];

/* ─── Rutas de imágenes ──────────────────────────────────
   Todas en la carpeta Imagenes/ con extensiones reales
   ──────────────────────────────────────────────────────── */
const IMG_EXT = {
  /* piezas de ensamblaje */
  chasis:         { ext: 'png' },
  carcasa_futbol: { ext: 'png' },
  tapa_futbol:    { ext: 'png' },
  carcasa_sumo:   { ext: 'png' },
  tapa_sumo:      { ext: 'png' },
  tornillo_m2:    { ext: 'jpg' },
  llantas:        { ext: 'png' },
  /* componentes electrónicos */
  esp32:          { ext: 'png' },
  motor_n20:      { ext: 'jpg' },
  vl53l0x:        { ext: 'webp' },
  drv8833:        { ext: 'png' },
  sg90:           { ext: 'png' },
  tca9548a:       { ext: 'jpg' },
  bateria18650:   { ext: 'jpg' },
  mp1584:         { ext: 'jpg' },
};

function imgPath(file) {
  const info = IMG_EXT[file] || { ext: 'jpg' };
  return `Imagenes/${file}.${info.ext}`;
}

/* ─── Estado de navegación ─────────────────────────────── */
let history = [];   // pila de pantallas para volver atrás
let currentScreen = 'screen-home';

/* ─── Inicialización ───────────────────────────────────── */
document.addEventListener('DOMContentLoaded', () => {
  createParticles();
  buildPiecesGallery();
  buildInfoSidebar();
  setupMainVideo();
});

/* ════════════════════════════════════════════════════════
   NAVEGACIÓN
   ════════════════════════════════════════════════════════ */
function goTo(screenId) {
  const prev = document.getElementById(currentScreen);
  const next = document.getElementById(screenId);
  if (!next || screenId === currentScreen) return;

  history.push(currentScreen);

  prev.classList.add('exit');
  setTimeout(() => {
    prev.classList.remove('active', 'exit');
  }, 420);

  next.classList.add('active');
  currentScreen = screenId;

  // Reproducir videos de ensamble al entrar
  if (screenId === 'screen-ensamble') {
    playEnsambleVideos();
  }
  // Pausar videos al salir del ensamble
  if (screenId !== 'screen-ensamble') {
    pauseEnsambleVideos();
  }
}

function goBack(fromScreen) {
  if (history.length === 0) { goHome(); return; }
  const target = history.pop();
  const prev = document.getElementById(fromScreen);
  const next = document.getElementById(target);

  prev.classList.add('exit');
  setTimeout(() => { prev.classList.remove('active', 'exit'); }, 420);
  next.classList.add('active');
  currentScreen = target;

  if (target !== 'screen-ensamble') pauseEnsambleVideos();
}

function goHome() {
  const prev = document.getElementById(currentScreen);
  const home = document.getElementById('screen-home');

  // Pausar/resetear video principal
  const mv = document.getElementById('main-video');
  mv.pause();
  mv.currentTime = 0;
  document.getElementById('post-video').style.display = 'none';
  mv.style.display = 'block';

  history = [];
  pauseEnsambleVideos();

  prev.classList.add('exit');
  setTimeout(() => { prev.classList.remove('active', 'exit'); }, 420);
  home.classList.add('active');
  currentScreen = 'screen-home';
}

/* ════════════════════════════════════════════════════════
   PANTALLA HOME — PARTÍCULAS
   ════════════════════════════════════════════════════════ */
function createParticles() {
  const container = document.getElementById('particles');
  const colors = ['#7c3aed', '#06b6d4', '#a78bfa', '#67e8f9', '#c4b5fd'];
  for (let i = 0; i < 30; i++) {
    const p = document.createElement('div');
    p.className = 'particle';
    const size = Math.random() * 6 + 2;
    p.style.cssText = `
      width: ${size}px;
      height: ${size}px;
      left: ${Math.random() * 100}%;
      background: ${colors[Math.floor(Math.random() * colors.length)]};
      animation-duration: ${8 + Math.random() * 14}s;
      animation-delay: ${Math.random() * 10}s;
      opacity: 0;
    `;
    container.appendChild(p);
  }
}

/* ════════════════════════════════════════════════════════
   VIDEO PRINCIPAL
   ════════════════════════════════════════════════════════ */
function startExperience() {
  goTo('screen-video');
  setTimeout(() => {
    const mv = document.getElementById('main-video');
    mv.play().catch(() => {
      // Si el navegador bloquea autoplay, mostrar directamente los botones
      showPostVideo();
    });
  }, 450);
}

function setupMainVideo() {
  const mv = document.getElementById('main-video');
  mv.addEventListener('ended', showPostVideo);
  mv.addEventListener('error', showPostVideo);
}

function showPostVideo() {
  const mv = document.getElementById('main-video');
  mv.style.display = 'none';
  const pv = document.getElementById('post-video');
  pv.style.display = 'flex';
}

function skipVideo() {
  const mv = document.getElementById('main-video');
  mv.pause();
  showPostVideo();
}

/* ════════════════════════════════════════════════════════
   CONTROLES DE VIDEO (ENSAMBLE)
   ════════════════════════════════════════════════════════ */
function playEnsambleVideos() {
  ['video-sumo', 'video-futbol'].forEach(id => {
    const v = document.getElementById(id);
    if (v) v.play().catch(() => {});
  });
  // Actualizar íconos de play/pause
  document.querySelectorAll('.play-icon').forEach(el => { el.textContent = '⏸'; });
}

function pauseEnsambleVideos() {
  ['video-sumo', 'video-futbol'].forEach(id => {
    const v = document.getElementById(id);
    if (v) v.pause();
  });
}

function togglePlay(videoId, btn) {
  const v = document.getElementById(videoId);
  const icon = btn.querySelector('.play-icon');
  if (v.paused) {
    v.play();
    icon.textContent = '⏸';
  } else {
    v.pause();
    icon.textContent = '▶';
  }
}

function toggleMute(videoId, btn) {
  const v = document.getElementById(videoId);
  const icon = btn.querySelector('.mute-icon');
  v.muted = !v.muted;
  icon.textContent = v.muted ? '🔇' : '🔊';
}

function restartVideo(videoId) {
  const v = document.getElementById(videoId);
  v.currentTime = 0;
  v.play().catch(() => {});
  // Actualizar ícono de play
  const card = v.closest('.video-card');
  if (card) card.querySelector('.play-icon').textContent = '⏸';
}

/* ════════════════════════════════════════════════════════
   GALERÍA DE PIEZAS (ENSAMBLE)
   ════════════════════════════════════════════════════════ */
function buildPiecesGallery() {
  const gallery = document.getElementById('pieces-gallery');
  if (!gallery) return;

  PIECES.forEach(piece => {
    const card = document.createElement('div');
    card.className = 'piece-card';
    card.onclick = () => openModal(imgPath(piece.file), piece.label);

    card.innerHTML = `
      <img src="${imgPath(piece.file)}" alt="${piece.label}" loading="lazy"
           onerror="this.src='data:image/svg+xml,${encodeURIComponent(placeholderSVG(piece.label))}'"/>
      <div class="piece-name">${piece.label}</div>
      <div class="expand-hint">🔍</div>
    `;
    gallery.appendChild(card);
  });
}

/* ════════════════════════════════════════════════════════
   SECCIÓN INFORMACIÓN — Sidebar + Detalle
   ════════════════════════════════════════════════════════ */
function buildInfoSidebar() {
  const sidebar = document.getElementById('info-sidebar');
  if (!sidebar) return;

  COMPONENTS.forEach((comp, idx) => {
    const tab = document.createElement('div');
    tab.className = 'component-tab';
    tab.id = `tab-${comp.id}`;
    tab.onclick = () => selectComponent(comp.id);

    tab.innerHTML = `
      <img class="tab-thumb" src="${imgPath(comp.file)}" alt="${comp.name}"
           onerror="this.src='data:image/svg+xml,${encodeURIComponent(placeholderSVG(comp.name))}'" />
      <div>
        <div class="tab-name">${comp.name}</div>
        <div class="tab-category">${comp.badge}</div>
      </div>
    `;
    sidebar.appendChild(tab);
  });
}

let selectedComponent = null;

function selectComponent(id) {
  // Actualizar tab activo
  document.querySelectorAll('.component-tab').forEach(t => t.classList.remove('active'));
  const tab = document.getElementById(`tab-${id}`);
  if (tab) tab.classList.add('active');

  const comp = COMPONENTS.find(c => c.id === id);
  if (!comp) return;
  selectedComponent = comp;

  const detail = document.getElementById('info-detail');
  detail.innerHTML = `
    <div class="detail-card">
      <div class="detail-hero">
        <div class="detail-img-wrap" onclick="openModal('${imgPath(comp.file)}', '${comp.name}')">
          <img src="${imgPath(comp.file)}" alt="${comp.name}"
               onerror="this.src='data:image/svg+xml,${encodeURIComponent(placeholderSVG(comp.name))}'"/>
          <div class="expand-hint">🔍</div>
        </div>
        <div class="detail-header">
          <h2>${comp.name}</h2>
          <div class="component-badge">${comp.badge}</div>
          <p>${comp.desc}</p>
        </div>
      </div>
      <div class="detail-blocks">
        <div class="info-block">
          <div class="info-block-icon">⚙️</div>
          <h4>¿Cómo funciona?</h4>
          <p>${comp.how}</p>
        </div>
        <div class="info-block">
          <div class="info-block-icon">⚡</div>
          <h4>Alimentación</h4>
          <p>${comp.power}</p>
        </div>
        <div class="info-block" style="grid-column: 1 / -1;">
          <div class="info-block-icon">🤖</div>
          <h4>Rol en el robot</h4>
          <p>${comp.role}</p>
        </div>
      </div>
    </div>
  `;
  // Scroll hacia arriba en el panel
  detail.scrollTop = 0;
}

/* ════════════════════════════════════════════════════════
   MODAL DE IMAGEN AMPLIADA
   ════════════════════════════════════════════════════════ */
function openModal(src, caption) {
  const modal  = document.getElementById('img-modal');
  const img    = document.getElementById('modal-img');
  const cap    = document.getElementById('modal-caption');
  img.src      = src;
  img.alt      = caption;
  cap.textContent = caption;
  modal.classList.add('open');
}

function closeModal() {
  document.getElementById('img-modal').classList.remove('open');
}

// Cerrar con ESC (útil en teclado conectado)
document.addEventListener('keydown', e => {
  if (e.key === 'Escape') closeModal();
});

/* ════════════════════════════════════════════════════════
   UTILIDADES
   ════════════════════════════════════════════════════════ */

/** SVG placeholder cuando no se encuentra la imagen */
function placeholderSVG(label) {
  return `<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200" viewBox="0 0 200 200">
    <rect width="200" height="200" fill="#1a1a3e"/>
    <text x="100" y="90" text-anchor="middle" fill="#7c3aed" font-size="48">📦</text>
    <text x="100" y="130" text-anchor="middle" fill="#8888aa" font-size="13" font-family="sans-serif">${label.substring(0,20)}</text>
  </svg>`;
}

/* ════════════════════════════════════════════════════════
   PANTALLA COMPLETA
   ════════════════════════════════════════════════════════ */
function toggleFullscreen() {
  const btn = document.getElementById('btn-fs');
  if (!document.fullscreenElement && !document.webkitFullscreenElement) {
    const el = document.documentElement;
    if (el.requestFullscreen)       el.requestFullscreen();
    else if (el.webkitRequestFullscreen) el.webkitRequestFullscreen();
    if (btn) btn.textContent = '⛶';
  } else {
    if (document.exitFullscreen)       document.exitFullscreen();
    else if (document.webkitExitFullscreen) document.webkitExitFullscreen();
    if (btn) btn.textContent = '⛶';
  }
}

// Actualizar ícono si el usuario sale de FS con Escape
document.addEventListener('fullscreenchange', () => {
  const btn = document.getElementById('btn-fs');
  if (!document.fullscreenElement && btn) btn.textContent = '⛶';
});
document.addEventListener('webkitfullscreenchange', () => {
  const btn = document.getElementById('btn-fs');
  if (!document.webkitFullscreenElement && btn) btn.textContent = '⛶';
});

/* ════════════════════════════════════════════════════════
   TECLAS RÁPIDAS (para presentadores con teclado)
   ════════════════════════════════════════════════════════ */
document.addEventListener('keydown', e => {
  switch (e.key) {
    case 'Escape':   closeModal(); break;
    case 'F11':      e.preventDefault(); toggleFullscreen(); break;
    case 'h':
    case 'H':        if (currentScreen !== 'screen-home') goHome(); break;
  }
});
