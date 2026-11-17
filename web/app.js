const $ = selector => document.querySelector(selector);
const icons = ['compass', 'tent', 'medical', 'water', 'torch', 'sleeping', 'stove', 'camera'];
const names = ['Навигатор', 'Палатка', 'Аптечка', 'Вода', 'Фонарик', 'Спальник', 'Горелка', 'Камера'];
const catalog = [
  ['Компас', 0], ['Палатка', 1], ['Аптечка', 2], ['Бутылка воды', 3],
  ['Фонарик', 4], ['Спальник', 5], ['Горелка', 6], ['Камера', 7],
  ['Навигатор', 0], ['Тент', 1], ['Перевязочный набор', 2], ['Термос', 3],
  ['Налобный фонарь', 4], ['Коврик', 5], ['Котелок', 6], ['Бинокль', 7],
  ['Карта', 0], ['Дождевик', 1], ['Набор инструментов', 2], ['Фляга', 3],
  ['Аккумулятор', 4], ['Плед', 5], ['Запас еды', 6], ['Рация', 7]
];
const backgrounds = ['#f6e4d7', '#e6edda', '#f5e2df', '#e0edf0', '#f7edcc', '#e9e3ee', '#e4ebe0', '#eae7df'];
const state = {
  items: [], selected: new Set(), capacity: 18, method: 'optimized', view: 'packing',
  editing: false, editId: null, results: {}, manual: true, busy: false, nextId: 1,
  highlightId: null, arrivedId: null, generator: { maxVolume: 12, maxValue: 100, seed: 2026 }
};
const assembly = { ids: [], index: 0, playing: false, inFlight: false, timer: null, version: 0 };
const traceView = { data: null, index: 0, playing: false, timer: null };
const pendingRequests = new Map();
let requestId = 0;
let toastTimer;
let busyTimer;
let currentFlight = null;

function icon(name) {
  return `<svg aria-hidden="true"><use href="#i-${name}"/></svg>`;
}

function illustration(type) {
  const drawings = {
    compass: '<circle cx="24" cy="25" r="15" fill="#efd3b8" stroke="#aa8063" stroke-width="1.5"/><circle cx="24" cy="25" r="11" fill="#faf4e8"/><path d="m28 16-1 12-8 6 2-12 7-6Z" fill="#9f5944"/><path d="m21 22 6 6-8 6 2-12Z" fill="#b2b9a4"/><circle cx="24" cy="25" r="2" fill="#6d6754"/><path d="M21 9V6h6v3" stroke="#aa8063" stroke-width="2"/>',
    tent: '<path d="m24 9 20 29H4L24 9Z" fill="#829c6e" stroke="#67825a" stroke-width="1.5"/><path d="m24 9 8 29H9L24 9Z" fill="#b9c69a"/><path d="m24 19 6 19H17l7-19Z" fill="#526c4b"/><path d="M24 5v8M5 40h38" stroke="#7f8060" stroke-width="1.5" stroke-linecap="round"/>',
    medical: '<path d="M18 15v-4h12v4" stroke="#ba7f75" stroke-width="3"/><rect x="7" y="15" width="34" height="25" rx="7" fill="#d2998c" stroke="#bb8074" stroke-width="1.5"/><path d="M21 21h6v5h5v6h-5v5h-6v-5h-5v-6h5v-5Z" fill="#fff5e8"/>',
    water: '<path d="M19 7h10v7l5 5v21H14V19l5-5V7Z" fill="#a3c8ce" stroke="#7aa7af" stroke-width="1.5"/><rect x="18" y="5" width="12" height="6" rx="2" fill="#648f99"/><path d="M16 26h16v11H16V26Z" fill="#d8e9e5"/><path d="M20 19v5" stroke="#eaf5ef" stroke-width="2" stroke-linecap="round"/>',
    torch: '<path d="m13 9 19-3 3 12-19 3-3-12Z" fill="#d8b967" stroke="#ae9249" stroke-width="1.5"/><path d="m19 20 11-2 4 21-11 2-4-21Z" fill="#b49d63" stroke="#99814a" stroke-width="1.5"/><path d="m16 11 3 9m7 4 2 10" stroke="#f7e6ae" stroke-width="2" stroke-linecap="round"/><path d="m36 6 5-2m-3 8h6" stroke="#c6aa59" stroke-width="1.5" stroke-linecap="round"/>',
    sleeping: '<rect x="12" y="5" width="24" height="38" rx="11" fill="#b4a3bd" stroke="#97839f" stroke-width="1.5"/><ellipse cx="24" cy="14" rx="8" ry="6" fill="#eee4e9"/><path d="M24 21v19M13 27h22M13 34h22" stroke="#94839e" stroke-width="1.5"/><path d="M29 22v4" stroke="#eee2e6" stroke-width="2"/>',
    stove: '<path d="M17 29h14l3 11H14l3-11Z" fill="#b9b7a3" stroke="#8c9480" stroke-width="1.5"/><path d="M11 27h26M24 23v6M14 36l-5 5m25-5 5 5" stroke="#7b8b70" stroke-width="2" stroke-linecap="round"/><path d="M25 6c2 8 9 8 7 15-1 6-13 6-15 0-2-6 3-9 8-15Z" fill="#d8a16e"/><path d="M24 14c0 4 5 5 4 8-1 3-7 3-8 0-1-3 3-5 4-8Z" fill="#f8d79b"/>',
    camera: '<rect x="6" y="16" width="36" height="24" rx="6" fill="#a8a494" stroke="#888976" stroke-width="1.5"/><path d="m16 16 3-6h12l4 6" fill="#c8c4b2" stroke="#888976" stroke-width="1.5"/><circle cx="25" cy="28" r="9" fill="#d9d8c8" stroke="#808775" stroke-width="1.5"/><circle cx="25" cy="28" r="5.5" fill="#657b70"/><circle cx="23" cy="26" r="2" fill="#b6c9bd"/><rect x="9" y="20" width="6" height="4" rx="1" fill="#e9dec2"/>'
  };
  return `<svg viewBox="0 0 48 48" fill="none" aria-hidden="true">${drawings[type] || drawings.compass}</svg>`;
}


function escapeHTML(value) {
  return String(value).replace(/[&<>"']/g, char => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[char]));
}

function makeItem(name, volume, value, type = 0) {
  return { id: state.nextId++, name, volume, value, type: type % icons.length };
}

function itemWord(number) {
  const last = number % 10;
  return number % 100 >= 11 && number % 100 <= 14 ? 'предметов' : last === 1 ? 'предмет' : last >= 2 && last <= 4 ? 'предмета' : 'предметов';
}

function formatTime(value) {
  return value < 0.001 ? value.toFixed(4) : value.toFixed(3);
}

function reducedMotion() {
  return window.matchMedia('(prefers-reduced-motion: reduce)').matches;
}

function toast(message) {
  clearTimeout(toastTimer);
  $('#toast').textContent = message;
  $('#toast').classList.add('visible');
  toastTimer = setTimeout(() => $('#toast').classList.remove('visible'), 3500);
}

async function requestBackend(action, data = {}) {
  if (!window.webkit?.messageHandlers?.knapsack) {
    if (action === 'export') {
      const blob = new Blob([data.csv], { type: 'text/csv;charset=utf-8' });
      const url = URL.createObjectURL(blob);
      const link = document.createElement('a');
      link.href = url;
      link.download = data.name || 'knapsack-results.csv';
      document.body.append(link);
      link.click();
      link.remove();
      setTimeout(() => URL.revokeObjectURL(url), 1000);
      return { saved: true };
    }
    if (window.location.protocol === 'file:') {
      throw new Error('Запустите python3 server.py и откройте адрес, указанный в терминале.');
    }
    let response;
    try {
      response = await fetch('/api', {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action, ...data })
      });
    } catch (error) {
      throw new Error('Нет связи с C++ программой. Проверьте, что server.py запущен.');
    }
    const result = await response.json();
    if (!response.ok || result.error) throw new Error(result.error || 'Не удалось выполнить запрос.');
    return result;
  }
  return new Promise((resolve, reject) => {
    const id = ++requestId;
    pendingRequests.set(id, { resolve, reject });
    window.webkit.messageHandlers.knapsack.postMessage({ id, action, ...data });
  });
}

window.onNativeMessage = message => {
  const request = pendingRequests.get(message.id);
  if (!request) return;
  pendingRequests.delete(message.id);
  if (message.error) request.reject(new Error(message.error));
  else request.resolve(message);
};

function selectionTotals() {
  return state.items.reduce((total, item) => {
    if (state.selected.has(item.id)) {
      total.volume += item.volume;
      total.value += item.value;
      total.count++;
    }
    return total;
  }, { volume: 0, value: 0, count: 0 });
}

function renderItems(newIds = new Set()) {
  const list = $('#item-list');
  const focusedId = document.activeElement?.dataset?.id;
  const step = state.view === 'trace' ? currentTraceStep() : null;
  const selected = step ? new Set(step.path.map(index => state.items[index].id)) : state.selected;
  const highlightId = step && step.item >= 0 ? state.items[step.item]?.id : state.highlightId;
  $('#item-count').textContent = state.items.length;
  list.classList.toggle('editing', state.editing);
  $('#edit-button').classList.toggle('active', state.editing);
  $('#inventory-hint').textContent = state.editing ? 'Измените предмет или удалите его из списка.' : state.view === 'trace' ? 'Подсвечены предметы текущего состояния поиска.' : 'Нажмите или перетащите предмет в рюкзак.';
  list.innerHTML = state.items.length ? state.items.map((item, index) => `
    <div class="item-row ${selected.has(item.id) ? 'selected' : ''} ${highlightId === item.id ? 'packing-now' : ''} ${newIds.has(item.id) ? 'new-item' : ''}" data-id="${item.id}" tabindex="${state.busy ? -1 : 0}" draggable="${!state.busy && !state.editing && state.view !== 'trace'}" role="button" aria-pressed="${selected.has(item.id)}" aria-label="${escapeHTML(item.name)}, объём ${item.volume}, стоимость ${item.value}">
      <span class="item-art" style="--item-bg:${backgrounds[item.type]}">${illustration(icons[item.type])}</span>
      <div class="item-copy"><strong>${escapeHTML(item.name)}</strong><small>№ ${String(index + 1).padStart(2, '0')}</small></div>
      <span class="item-volume">${item.volume}</span><span class="item-value">${item.value}</span>
      <span class="item-toggle">${selected.has(item.id) ? icon('check') : ''}</span>
      <button class="item-edit" data-edit="${item.id}" title="Изменить предмет" aria-label="Изменить ${escapeHTML(item.name)}">${icon('edit')}</button>
      <button class="item-delete" data-delete="${item.id}" title="Удалить предмет" aria-label="Удалить ${escapeHTML(item.name)}">${icon('close')}</button>
    </div>`).join('') : '<div class="empty-inventory">Предметов пока нет.<br>Добавьте их вручную или создайте новый набор.</div>';
  if (focusedId) list.querySelector(`[data-id="${focusedId}"]`)?.focus({ preventScroll: true });
}

function renderPacked(incoming = null) {
  const selected = state.items.filter(item => state.selected.has(item.id));
  if (incoming && !state.selected.has(incoming.id)) selected.push(incoming);
  $('#packed-items').innerHTML = selected.slice(0, 8).map(item => `<button class="packed-item ${item.id === state.arrivedId ? 'arrived' : ''}" data-packed="${item.id}" title="${escapeHTML(item.name)} · убрать" aria-label="Убрать ${escapeHTML(item.name)}" style="--item-bg:${backgrounds[item.type]};${incoming?.id === item.id ? 'visibility:hidden' : ''}">${illustration(icons[item.type])}</button>`).join('') + (selected.length > 8 ? `<span class="packed-item more-items">+${selected.length - 8}</span>` : '');
  $('#empty-pack-label').hidden = selected.length > 0;
}

function renderAssembly() {
  const total = assembly.ids.length;
  const done = total > 0 && assembly.index >= total;
  const currentId = assembly.ids[assembly.inFlight ? assembly.index : assembly.index - 1];
  const current = state.items.find(item => item.id === currentId);
  let status = 'Запустите поиск или соберите набор вручную.';
  if (!state.manual && total === 0) status = 'Оптимальный набор пуст.';
  if (current && assembly.inFlight) status = `Добавляем «${current.name}»: объём +${current.volume}, стоимость ${current.value >= 0 ? '+' : ''}${current.value}.`;
  else if (done) status = `Набор собран: ${total} ${itemWord(total)}.`;
  else if (current) status = `Добавлен «${current.name}». ${assembly.playing ? '' : 'Пауза.'}`;
  else if (total) status = 'Сборка найденного набора.';
  $('#assembly-status').textContent = status;
  $('#assembly-progress').textContent = total ? `${assembly.index} / ${total}` : '';
  $('#playback-button').innerHTML = `${icon(assembly.playing ? 'pause' : 'play')}<span>${assembly.playing ? 'Пауза' : done ? 'Повторить' : assembly.index ? 'Продолжить' : 'Показать'}</span>`;
  $('#playback-button').disabled = state.busy || total === 0;
  $('#step-button').disabled = state.busy || total === 0 || assembly.inFlight || assembly.playing;
  $('#finish-button').disabled = state.busy || total === 0 || done;
  $('#playback-speed').disabled = state.busy;
}

function renderSelection() {
  const totals = selectionTotals();
  renderPacked();
  $('#scene-selected-count').textContent = totals.count;
  $('#scene-count-word').textContent = itemWord(totals.count);
  $('#total-value').textContent = totals.value.toLocaleString('ru-RU');
  $('#total-volume').innerHTML = `${totals.volume.toLocaleString('ru-RU')} <small>/ ${state.capacity.toLocaleString('ru-RU')}</small>`;
  const percent = state.capacity ? Math.min(100, totals.volume / state.capacity * 100) : 0;
  $('#volume-fill').style.width = `${percent}%`;
  $('#result-status').textContent = state.busy ? 'Расчёт…' : assembly.ids.length && assembly.index < assembly.ids.length ? 'Сборка решения' : state.manual ? totals.count ? 'Свой набор' : 'Рюкзак пуст' : 'Оптимальный набор';
  const result = state.results[state.method === 'brute' ? 'brute' : 'optimized'];
  $('#solve-time').innerHTML = !state.manual && result && !result.skipped ? `${formatTime(result.timeMs)} <small>мс</small>` : '— <small>мс</small>';
  $('#time-caption').textContent = 'Медиана 5 запусков';
  renderAssembly();
}

function renderComparison() {
  $('#comparison-dataset').textContent = `${state.items.length} ${itemWord(state.items.length)} · ${state.capacity} ед.`;
  const optimized = state.results.optimized;
  const brute = state.results.brute;
  const maximum = Math.max(optimized?.timeMs || 0, brute?.timeMs || 0, 0.000001);
  for (const [name, result] of [['optimized', optimized], ['brute', brute]]) {
    $(`#${name}-time`).textContent = result && !result.skipped ? formatTime(result.timeMs) : '—';
    $(`#${name}-bar`).style.width = result && !result.skipped ? `${result.timeMs / maximum * 100}%` : '0';
    $(`#${name}-details`).textContent = result?.skipped ? 'Пропущен: больше 28 предметов.' : result ? `Стоимость ${result.cost} · объём ${result.volume} · ${result.indices.length} ${itemWord(result.indices.length)}` : 'Нет результатов';
  }
  let title = 'Нет результатов сравнения.';
  let description = 'Запустите оба метода на текущем наборе.';
  if (optimized && brute && !brute.skipped) {
    title = `Ответы совпадают. Максимальная стоимость — ${optimized.cost}.`;
    if (optimized.timeMs > 0 && brute.timeMs > 0) {
      const faster = optimized.timeMs <= brute.timeMs ? 'Оптимизированный поиск' : 'Полный перебор';
      const ratio = Math.max(optimized.timeMs, brute.timeMs) / Math.min(optimized.timeMs, brute.timeMs);
      description = `${faster} быстрее в ${ratio.toFixed(2)} раза на этом наборе.`;
    } else description = 'Оба метода нашли оптимальный набор.';
  } else if (brute?.skipped) {
    title = `Максимальная стоимость — ${optimized.cost}.`;
    description = 'Для сравнения с полным перебором уменьшите набор до 28 предметов.';
  }
  $('#comparison-verdict').innerHTML = `${icon(optimized && brute && !brute.skipped ? 'check' : 'info')}<div><strong>${title}</strong><p>${description}</p></div>`;
}

function updateMethod() {
  document.querySelectorAll('.method-option').forEach(button => {
    button.classList.toggle('active', button.dataset.method === state.method);
    button.disabled = state.busy || (state.view === 'trace' && button.dataset.method === 'compare');
  });
  $('#solve-button span').textContent = state.busy ? 'Подождите…' : state.view === 'trace' ? 'Записать ход' : state.method === 'compare' ? 'Сравнить' : 'Решить';
  if (!state.busy) $('#run-description').textContent = 'Метод поиска';
}

function render() {
  renderItems();
  renderSelection();
  renderComparison();
  renderTrace();
  $('#capacity-input').value = state.capacity;
  $('#capacity-slider').max = Math.max(50, state.capacity, state.items.reduce((sum, item) => sum + item.volume, 0));
  $('#capacity-slider').value = state.capacity;
  const fill = state.capacity / Number($('#capacity-slider').max) * 100;
  $('#capacity-slider').style.background = `linear-gradient(to right, #8fa77b ${fill}%, #e8ede2 ${fill}%)`;
  updateMethod();
}

function stopAssembly(clear = true) {
  assembly.version++;
  assembly.playing = false;
  assembly.inFlight = false;
  clearTimeout(assembly.timer);
  assembly.timer = null;
  if (currentFlight) {
    currentFlight.animation.cancel();
    currentFlight.element.remove();
    currentFlight = null;
  }
  state.highlightId = null;
  state.arrivedId = null;
  if (clear) {
    assembly.ids = [];
    assembly.index = 0;
  }
}

function invalidate() {
  stopAssembly();
  resetTrace();
  state.results = {};
  state.selected.clear();
  state.manual = true;
  $('#scenario-select').value = 'custom';
}

async function flyItem(item, source, destination) {
  if (reducedMotion() || !source || !destination) return;
  const from = source.getBoundingClientRect();
  const to = destination.getBoundingClientRect();
  if (!from.width || !to.width) return;
  const element = document.createElement('div');
  element.className = 'flying-item';
  element.style.setProperty('--item-bg', backgrounds[item.type]);
  element.style.left = `${from.left + from.width / 2 - 18}px`;
  element.style.top = `${from.top + from.height / 2 - 18}px`;
  element.innerHTML = illustration(icons[item.type]);
  document.body.append(element);
  const dx = to.left + to.width / 2 - from.left - from.width / 2;
  const dy = to.top + to.height / 2 - from.top - from.height / 2;
  const speed = Number($('#playback-speed').value) || 1;
  const animation = element.animate([
    { transform: 'translate(0, 0) scale(1)', opacity: 1 },
    { offset: .55, transform: `translate(${dx * .55}px, ${dy * .55 - 45}px) rotate(9deg) scale(.9)`, opacity: 1 },
    { transform: `translate(${dx}px, ${dy}px) rotate(0deg) scale(.72)`, opacity: .95 }
  ], { duration: 460 / speed, easing: 'cubic-bezier(.3,.05,.25,1)', fill: 'forwards' });
  const flight = { element, animation };
  currentFlight = flight;
  try { await animation.finished; } catch (error) { if (error.name !== 'AbortError') throw error; }
  element.remove();
  if (currentFlight === flight) currentFlight = null;
}

async function placeItem(id, version) {
  const item = state.items.find(item => item.id === id);
  if (!item || state.selected.has(id)) return false;
  state.highlightId = id;
  renderItems();
  renderPacked(item);
  const row = $(`[data-id="${id}"]`);
  const list = $('#item-list');
  if (row && window.innerWidth > 760) {
    const box = row.getBoundingClientRect();
    const listBox = list.getBoundingClientRect();
    if (box.top < listBox.top || box.bottom > listBox.bottom) list.scrollTop += box.top - listBox.top - listBox.height / 2 + box.height / 2;
  }
  renderAssembly();
  await flyItem(item, row?.querySelector('.item-art'), $(`[data-packed="${id}"]`) || $('#packed-items'));
  if (version !== assembly.version) return false;
  state.selected.add(id);
  state.highlightId = null;
  state.arrivedId = id;
  renderItems();
  renderSelection();
  state.arrivedId = null;
  if (!reducedMotion()) $('#rucksack').animate([{ transform: 'translateY(0)' }, { transform: 'translateY(3px)' }, { transform: 'translateY(0)' }], { duration: 200 });
  return true;
}

async function advanceAssembly() {
  if (assembly.inFlight || assembly.index >= assembly.ids.length) return;
  assembly.inFlight = true;
  const version = assembly.version;
  const placed = await placeItem(assembly.ids[assembly.index], version);
  if (version !== assembly.version) return;
  if (placed) assembly.index++;
  assembly.inFlight = false;
  if (!placed || assembly.index >= assembly.ids.length) assembly.playing = false;
  renderSelection();
  if (assembly.playing) {
    const speed = Number($('#playback-speed').value) || 1;
    assembly.timer = setTimeout(advanceAssembly, reducedMotion() ? 0 : 260 / speed);
  }
}

function startAssembly() {
  if (state.busy || assembly.ids.length === 0) return;
  if (assembly.index >= assembly.ids.length) {
    stopAssembly(false);
    assembly.index = 0;
    state.selected.clear();
    state.manual = false;
    renderItems();
    renderSelection();
  }
  assembly.playing = true;
  renderAssembly();
  if (!assembly.inFlight) advanceAssembly();
}

function pauseAssembly() {
  assembly.playing = false;
  clearTimeout(assembly.timer);
  assembly.timer = null;
  renderAssembly();
}

function finishAssembly() {
  if (assembly.ids.length === 0) return;
  stopAssembly(false);
  assembly.index = assembly.ids.length;
  state.selected = new Set(assembly.ids);
  state.manual = false;
  renderItems();
  renderSelection();
}

function beginAssembly(result, autoplay) {
  stopAssembly();
  assembly.ids = result.indices.map(index => state.items[index].id);
  state.selected.clear();
  state.manual = false;
  render();
  if (autoplay) startAssembly();
  else if (assembly.ids.length) finishAssembly();
}

async function toggleItem(id) {
  if (state.busy) return;
  if (state.editing) { openItemDialog(id); return; }
  if (state.view === 'trace') { pauseTrace(); return; }
  const item = state.items.find(item => item.id === id);
  if (!item) return;
  if (!state.selected.has(id) && selectionTotals().volume + item.volume > state.capacity) {
    toast('Предмет не помещается. Уберите что-нибудь или увеличьте вместимость.');
    $('#drop-zone').classList.remove('rejected');
    void $('#drop-zone').offsetWidth;
    $('#drop-zone').classList.add('rejected');
    return;
  }
  stopAssembly();
  state.manual = true;
  if (state.selected.has(id)) {
    state.selected.delete(id);
    renderItems();
    renderSelection();
  } else await placeItem(id, assembly.version);
}

function setBusy(value, generating = false) {
  state.busy = value;
  document.body.classList.toggle('busy-generating', value && generating);
  document.querySelectorAll('button, input, select').forEach(element => element.disabled = value);
  $('#solve-button').classList.toggle('loading', value && !generating);
  clearInterval(busyTimer);
  if (value) {
    const start = Date.now();
    $('#run-description').textContent = generating ? 'Генерация предметов…' : state.view === 'trace' ? 'Запись хода поиска…' : 'Выполняется поиск…';
    if (!generating) busyTimer = setInterval(() => $('#run-description').textContent = `${state.view === 'trace' ? 'Запись' : 'Поиск'} · ${((Date.now() - start) / 1000).toFixed(1)} с`, 100);
  }
  updateMethod();
  renderAssembly();
  renderTrace();
  renderStructureLab();
}

async function solve(method = state.method) {
  if (state.busy) return;
  if (method === 'brute' && state.items.length > 28) {
    toast('Для полного перебора выберите не более 28 предметов.');
    return;
  }
  stopAssembly();
  state.method = method;
  setBusy(true);
  renderItems();
  renderSelection();
  let result = null;
  try {
    const response = await requestBackend('solve', { method, capacity: state.capacity, items: state.items.map(({ volume, value }) => ({ volume, value })) });
    state.results = { ...state.results, ...response.results };
    result = response.results[method === 'brute' ? 'brute' : 'optimized'];
  } catch (error) { toast(error.message); }
  finally { setBusy(false); }
  if (result) beginAssembly(result, state.view === 'packing');
  else render();
}

function freshSeed() {
  const value = new Uint32Array(1);
  window.crypto.getRandomValues(value);
  return value[0] % 2147483647;
}

function randomItems(count, maxVolume, maxValue, seed) {
  let current = seed % 2147483646 + 1;
  const random = () => { current = current * 48271 % 2147483647; return current; };
  const pool = catalog.map(item => [...item]);
  for (let i = pool.length - 1; i > 0; --i) {
    const other = random() % (i + 1);
    [pool[i], pool[other]] = [pool[other], pool[i]];
  }
  return Array.from({ length: count }, (_, index) => {
    const [name, type] = pool[index % pool.length];
    const suffix = index >= pool.length ? ` ${Math.floor(index / pool.length) + 1}` : '';
    return makeItem(name + suffix, 1 + random() % maxVolume, 1 + random() % maxValue, type);
  });
}

async function generateItems(options) {
  if (state.busy) return;
  const { count, capacity, maxVolume, maxValue, seed } = options;
  if (![count, capacity, maxVolume, maxValue, seed].every(Number.isInteger)
      || count < 0 || count > 200 || capacity < 0 || capacity > 1000000
      || maxVolume < 1 || maxVolume > 1000000 || maxValue < 1 || maxValue > 1000000
      || seed < 0 || seed > 2147483647) {
    toast('Проверьте параметры генерации. Количество предметов — от 0 до 200.');
    return;
  }
  const items = randomItems(count, maxVolume, maxValue, seed);
  invalidate();
  state.items = [];
  state.capacity = capacity;
  state.editing = false;
  state.generator = { maxVolume, maxValue, seed };
  $('#quick-count').value = count;
  const form = $('#generate-form');
  for (const [name, value] of Object.entries(options)) form.elements[name].value = value;
  render();
  setBusy(true, true);
  try {
    const batchSize = Math.max(1, Math.ceil(count / 16));
    for (let i = 0; i < items.length; i += batchSize) {
      const batch = items.slice(i, i + batchSize);
      state.items.push(...batch);
      renderItems(new Set(batch.map(item => item.id)));
      $('#generation-status').textContent = `${state.items.length} / ${count}`;
      if (!reducedMotion()) await new Promise(resolve => setTimeout(resolve, 35));
    }
  } finally {
    setBusy(false);
    $('#generation-status').textContent = 'Предмет';
    render();
  }
}

function applyScenario(name) {
  invalidate();
  state.editing = false;
  if (name === 'tutorial') {
    state.items = [makeItem('Аптечка', 2, 3, 2), makeItem('Фонарик', 2, 8, 4), makeItem('Палатка', 3, 9, 1), makeItem('Вода', 1, 4, 3)];
    state.capacity = 4;
  } else if (name === 'large') {
    state.items = randomItems(40, 15, 100, 2026);
    state.capacity = 80;
  } else if (name === 'city') {
    state.items = [makeItem('Камера', 4, 95, 7), makeItem('Вода', 2, 45, 3), makeItem('Аптечка', 2, 60, 2), makeItem('Навигатор', 3, 80, 0), makeItem('Фонарик', 1, 25, 4), makeItem('Термос', 3, 55, 6)];
    state.capacity = 9;
  } else {
    state.items = [[3, 90], [8, 120], [2, 75], [4, 55], [1, 40], [6, 85], [3, 60], [4, 95]].map(([volume, value], index) => makeItem(names[index], volume, value, index));
    state.capacity = 18;
  }
  $('#quick-count').value = state.items.length;
  $('#scenario-select').value = name;
  $('#generation-status').textContent = 'Предмет';
  render();
}

function showView(view) {
  pauseAssembly();
  pauseTrace();
  pauseStructureLab();
  if (assembly.inFlight) {
    stopAssembly(false);
    renderItems();
    renderSelection();
  }
  state.view = view;
  document.body.classList.toggle('trace-mode', view === 'trace');
  $('#packing-view').hidden = view !== 'packing' && view !== 'trace';
  $('#packing-area').hidden = view !== 'packing';
  $('#trace-area').hidden = view !== 'trace';
  $('#comparison-view').hidden = view !== 'comparison';
  $('#structures-view').hidden = view !== 'structures';
  $('.scenario-picker').hidden = view === 'structures';
  $('.run-bar').hidden = view === 'structures';
  document.querySelectorAll('.nav-tab').forEach(button => button.classList.toggle('active', button.dataset.view === view));
  $('#page-heading').textContent = view === 'packing' ? 'Задача о рюкзаке' : view === 'trace' ? 'Ход решения' : view === 'structures' ? 'Структуры данных' : 'Сравнение алгоритмов';
  $('#page-description').textContent = view === 'packing' ? 'Максимальная стоимость при ограниченном объёме.' : view === 'trace' ? 'Воспроизведение реальных шагов поиска, записанных в C++.' : view === 'structures' ? 'Свои значения, операции и проверки по шагам.' : 'Один набор предметов и одинаковая вместимость.';
  if (view === 'comparison') state.method = 'compare';
  if (view === 'trace' && state.method === 'compare') state.method = 'optimized';
  if (traceView.data && traceView.data.method !== state.method && view === 'trace') resetTrace();
  updateMethod();
  renderComparison();
  renderItems();
  renderTrace();
}

function openItemDialog(id = null) {
  if (state.busy) return;
  pauseAssembly();
  pauseTrace();
  if (id === null && state.items.length >= 200) { toast('В одном наборе можно использовать до 200 предметов.'); return; }
  state.editId = id;
  const form = $('#item-form');
  form.reset();
  if (id !== null) {
    const item = state.items.find(item => item.id === id);
    for (const field of ['name', 'volume', 'value']) form.elements[field].value = item[field];
  }
  $('#item-dialog-title').textContent = id === null ? 'Добавить предмет' : 'Изменить предмет';
  form.querySelector('button[type="submit"]').innerHTML = `${id === null ? 'Добавить' : 'Сохранить'}${icon(id === null ? 'plus' : 'check')}`;
  $('#item-dialog').showModal();
}

function setCapacity(value) {
  if (!Number.isInteger(value) || value < 0 || value > 1000000) { toast('Вместимость — целое число от 0 до 1 000 000.'); render(); return; }
  if (value === state.capacity) return;
  state.capacity = value;
  invalidate();
  render();
}

function makeCSV() {
  const rows = [['Задача о рюкзаке'], ['Вместимость', state.capacity], [], ['Номер', 'Предмет', 'Объём', 'Стоимость']];
  state.items.forEach((item, index) => rows.push([index + 1, item.name, item.volume, item.value]));
  rows.push([], ['Метод', 'Медианное время, мс', 'Число запусков', 'Общая стоимость', 'Общий объём', 'Выбранные номера']);
  for (const [name, result] of Object.entries(state.results)) {
    const label = name === 'optimized' ? 'Оптимизированный поиск' : 'Полный перебор';
    rows.push(result.skipped ? [label, 'Пропущен: больше 28 предметов'] : [label, result.timeMs, result.repetitions, result.cost, result.volume, result.indices.map(index => index + 1).join(' ')]);
  }
  if (traceView.data) {
    const trace = traceView.data;
    rows.push([`Ход решения: ${trace.method === 'optimized' ? 'оптимизированный поиск' : 'полный перебор'}`, '', '', trace.result.cost, trace.result.volume, trace.result.indices.map(index => index + 1).join(' ')]);
    rows.push([], ['Ход решения', trace.truncated ? `Первые ${trace.limit} событий и итог` : 'Полная запись', 'Всего событий', trace.totalEvents]);
    rows.push(['Событие', 'Действие', 'Состояние', 'Родитель', 'Рассмотрено предметов', 'Объём', 'Стоимость', 'Верхняя оценка', 'Лучшая стоимость', 'Рассмотрено состояний', 'Отсечено', 'В очереди', 'Выбранные предметы']);
    for (const step of trace.steps) {
      rows.push([step.number, step.type, step.node, step.parent, step.index, step.volume, step.cost, step.bound ?? '', step.bestCost, step.visited, step.pruned, step.pending ?? '', step.path.map(index => index + 1).join(' ')]);
    }
  }
  const current = selectionTotals();
  rows.push(['Текущий набор', '', '', current.value, current.volume, state.items.map((item, index) => state.selected.has(item.id) ? index + 1 : null).filter(index => index !== null).join(' ')]);
  return '\uFEFF' + rows.map(row => row.map(value => `"${String(value).replaceAll('"', '""')}"`).join(',')).join('\r\n');
}

$('#item-list').addEventListener('click', event => {
  if (state.busy) return;
  const edit = event.target.closest('[data-edit]');
  const remove = event.target.closest('[data-delete]');
  if (edit) openItemDialog(Number(edit.dataset.edit));
  else if (remove) {
    state.items = state.items.filter(item => item.id !== Number(remove.dataset.delete));
    invalidate();
    render();
  } else {
    const row = event.target.closest('[data-id]');
    if (row) toggleItem(Number(row.dataset.id));
  }
});
$('#item-list').addEventListener('keydown', event => {
  if ((event.key === 'Enter' || event.key === ' ') && event.target.matches('[data-id]')) {
    event.preventDefault();
    toggleItem(Number(event.target.dataset.id));
  }
});
$('#item-list').addEventListener('dragstart', event => {
  const row = event.target.closest('[data-id]');
  if (!row || state.busy || state.editing || state.view === 'trace') { event.preventDefault(); return; }
  pauseAssembly();
  event.dataTransfer.setData('application/x-knapsack-item', row.dataset.id);
  event.dataTransfer.effectAllowed = 'copy';
  row.classList.add('dragging');
});
$('#item-list').addEventListener('dragend', () => {
  document.querySelectorAll('.dragging').forEach(row => row.classList.remove('dragging'));
  $('#drop-zone').classList.remove('drag-over');
});
$('#drop-zone').addEventListener('dragover', event => {
  if (state.busy || !Array.from(event.dataTransfer.types).includes('application/x-knapsack-item')) return;
  event.preventDefault();
  event.dataTransfer.dropEffect = 'copy';
  $('#drop-zone').classList.add('drag-over');
});
$('#drop-zone').addEventListener('dragleave', event => { if (!$('#drop-zone').contains(event.relatedTarget)) $('#drop-zone').classList.remove('drag-over'); });
$('#drop-zone').addEventListener('drop', event => {
  event.preventDefault();
  $('#drop-zone').classList.remove('drag-over');
  const id = Number(event.dataTransfer.getData('application/x-knapsack-item'));
  if (!state.selected.has(id)) toggleItem(id);
});
$('#packed-items').addEventListener('click', event => {
  const item = event.target.closest('[data-packed]');
  if (item && state.selected.has(Number(item.dataset.packed))) toggleItem(Number(item.dataset.packed));
});
$('#playback-button').addEventListener('click', () => assembly.playing ? pauseAssembly() : startAssembly());
$('#step-button').addEventListener('click', () => {
  if (assembly.index >= assembly.ids.length) {
    stopAssembly(false);
    assembly.index = 0;
    state.selected.clear();
    state.manual = false;
    renderItems();
  }
  advanceAssembly();
});
$('#finish-button').addEventListener('click', finishAssembly);
$('#edit-button').addEventListener('click', () => { pauseAssembly(); pauseTrace(); state.editing = !state.editing; renderItems(); });
$('#add-button').addEventListener('click', () => openItemDialog());
$('#empty-button').addEventListener('click', () => { stopAssembly(); state.selected.clear(); state.manual = true; renderItems(); renderSelection(); });
$('#solve-button').addEventListener('click', () => state.view === 'trace' ? recordTrace() : solve());
$('#capacity-slider').addEventListener('input', event => setCapacity(Number(event.target.value)));
$('#capacity-input').addEventListener('change', event => setCapacity(event.target.value.trim() === '' ? NaN : Number(event.target.value)));
$('#scenario-select').addEventListener('change', event => applyScenario(event.target.value));
document.querySelectorAll('[data-view]').forEach(button => button.addEventListener('click', () => showView(button.dataset.view)));
document.querySelectorAll('[data-method]').forEach(button => button.addEventListener('click', () => {
  pauseAssembly();
  if (state.view === 'trace' && state.method !== button.dataset.method) resetTrace();
  state.method = button.dataset.method;
  updateMethod();
  renderTrace();
  renderItems();
}));
document.querySelector('.brand').addEventListener('click', event => { event.preventDefault(); showView('packing'); });
document.querySelectorAll('[data-close-dialog]').forEach(button => button.addEventListener('click', () => button.closest('dialog').close()));
document.querySelectorAll('dialog').forEach(dialog => dialog.addEventListener('click', event => {
  if (event.target !== dialog) return;
  const rect = dialog.getBoundingClientRect();
  if (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom) dialog.close();
}));
$('#generate-button').addEventListener('click', () => {
  pauseAssembly();
  pauseTrace();
  const form = $('#generate-form');
  form.elements.count.value = $('#quick-count').value;
  form.elements.capacity.value = state.capacity;
  for (const [name, value] of Object.entries(state.generator)) form.elements[name].value = value;
  $('#generate-dialog').showModal();
});
$('#new-seed-button').addEventListener('click', () => { $('#generate-form').elements.seed.value = freshSeed(); });
$('#quick-generate').addEventListener('click', () => generateItems({
  count: $('#quick-count').value.trim() === '' ? NaN : Number($('#quick-count').value),
  capacity: state.capacity, ...state.generator, seed: freshSeed()
}));
$('#quick-count').addEventListener('keydown', event => {
  if (event.key === 'Enter') {
    event.preventDefault();
    $('#quick-generate').click();
  }
});
$('#generate-form').addEventListener('submit', event => {
  event.preventDefault();
  const fields = Object.fromEntries(new FormData(event.target));
  $('#generate-dialog').close();
  generateItems(Object.fromEntries(Object.entries(fields).map(([name, value]) => [name, Number(value)])));
});
$('#item-form').addEventListener('submit', event => {
  event.preventDefault();
  const fields = Object.fromEntries(new FormData(event.target));
  const name = fields.name.trim();
  if (!name) return;
  if (state.editId !== null) {
    const item = state.items.find(item => item.id === state.editId);
    Object.assign(item, { name, volume: Number(fields.volume), value: Number(fields.value) });
  } else state.items.push(makeItem(name, Number(fields.volume), Number(fields.value), state.items.length));
  invalidate();
  $('#item-dialog').close();
  render();
});
$('#export-button').addEventListener('click', async () => {
  try {
    const response = await requestBackend('export', state.view === 'structures'
      ? { csv: structureCSV(), name: 'structure-tests.csv' } : { csv: makeCSV() });
    if (response.saved) toast('CSV сохранён.');
  } catch (error) { toast(error.message); }
});

bindTraceControls();
bindStructureControls();
applyScenario('mountains');
$('#server-notice').hidden = window.location.protocol !== 'file:' || !!window.webkit?.messageHandlers?.knapsack;
