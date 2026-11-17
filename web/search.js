function currentTraceStep() {
  return traceView.data?.steps[traceView.index] || null;
}

function traceDescription(step) {
  const name = step.item >= 0 ? state.items[step.item]?.name : null;
  const item = name ? `«${name}»` : 'следующий предмет';
  const optimized = traceView.data.method === 'optimized';
  const messages = {
    start: ['Пустой рюкзак', optimized ? 'Корневое состояние добавлено в очередь. В таблице сохранена стоимость 0 для ключа (0, 0).' : 'Начинаем рекурсивный перебор с пустого рюкзака.'],
    visit: ['Рассматриваем состояние', `${optimized ? 'Состояние извлечено из очереди по приоритету верхней оценки.' : 'Вход в рекурсивный вызов.'} ${name ? `Следующий предмет — ${item}.` : 'Все предметы уже рассмотрены.'}`],
    best: ['Новая лучшая стоимость', `Найден допустимый набор стоимостью ${step.bestCost}. Запоминаем его как лучший из рассмотренных.`],
    skip: ['Ветвь «пропустить»', `${item} пропущен. Объём и стоимость не меняются. ${optimized ? 'Новое или улучшенное состояние добавлено в очередь.' : 'Переходим в рекурсивный вызов для следующего предмета.'}`],
    take: ['Ветвь «взять»', `${item} помещается. ${optimized ? 'Новое или улучшенное состояние добавлено в очередь.' : 'Переходим в рекурсивный вызов с этим предметом в рюкзаке.'}`],
    capacity: ['Предмет не помещается', `${item}: ${step.volume} + ${state.items[step.item]?.volume ?? 0} > ${state.capacity}. Ветвь «взять» не создаётся.`],
    duplicate: ['Состояние отброшено', `Для ключа (${step.index}, ${step.volume}) в таблице уже есть стоимость ${step.storedCost} ≥ ${step.cost}. Этот узел не добавляется в очередь.`],
    bound: ['Отсечение по верхней оценке', `Даже верхняя оценка ${step.bound} не больше лучшей стоимости ${step.bestCost}. Расширение этой ветви не улучшит ответ.`],
    stale: ['В очереди оказалось устаревшее состояние', `В таблице уже записана стоимость ${step.storedCost} > ${step.cost} для того же ключа. Узел пропускается.`],
    leaf: ['Конец ветви', 'Все предметы рассмотрены. Это допустимый конечный набор.'],
    backtrack: ['Возврат из рекурсии', `Возвращаемся в состояние № ${step.node} после обхода дочерней ветви.`],
    finish: ['Поиск завершён', `Максимальная стоимость — ${step.bestCost}. Рассмотрено состояний: ${step.visited}, отсечено ветвей: ${step.pruned}.`]
  };
  return messages[step.type] || ['Шаг поиска', ''];
}

function pauseTrace() {
  traceView.playing = false;
  clearTimeout(traceView.timer);
  traceView.timer = null;
  renderTrace();
}

function resetTrace() {
  traceView.playing = false;
  clearTimeout(traceView.timer);
  traceView.timer = null;
  traceView.data = null;
  traceView.index = 0;
  renderTrace();
}

function seekTrace(index) {
  if (!traceView.data) return;
  traceView.index = Math.max(0, Math.min(index, traceView.data.steps.length - 1));
  renderTrace(true);
  renderItems();
}

function playTrace() {
  if (!traceView.data || state.busy) return;
  if (traceView.index >= traceView.data.steps.length - 1) seekTrace(0);
  traceView.playing = true;
  renderTrace();
  scheduleTraceStep();
}

function scheduleTraceStep() {
  if (!traceView.playing || !traceView.data) return;
  const delay = 650 / Number($('#trace-speed').value || 1);
  traceView.timer = setTimeout(() => {
    if (!traceView.playing) return;
    seekTrace(traceView.index + 1);
    if (traceView.index >= traceView.data.steps.length - 1) pauseTrace();
    else scheduleTraceStep();
  }, delay);
}

async function recordTrace() {
  if (state.busy) return;
  if (state.method === 'compare') state.method = 'optimized';
  if (state.method === 'brute' && state.items.length > 28) {
    toast('Для полного перебора выберите не более 28 предметов.');
    return;
  }
  pauseAssembly();
  stopAssembly(false);
  resetTrace();
  setBusy(true);
  renderItems();
  try {
    const response = await requestBackend('trace', {
      method: state.method, capacity: state.capacity,
      items: state.items.map(({ volume, value }) => ({ volume, value }))
    });
    traceView.data = response.trace;
    traceView.index = 0;
  } catch (error) { toast(error.message); }
  finally {
    setBusy(false);
    render();
    renderTrace(true);
  }
}

function renderTraceChoices(step) {
  const branchEvent = ['skip', 'take', 'duplicate'].includes(step.type);
  const visible = step.item >= 0 && (branchEvent || ['visit', 'capacity'].includes(step.type));
  $('#trace-choices').hidden = !visible;
  if (!visible) return;
  const item = state.items[step.item];
  let volume = step.volume;
  let cost = step.cost;
  if (branchEvent && step.take) {
    volume -= item.volume;
    cost -= item.value;
  }
  $('#trace-skip-detail').textContent = `Объём ${volume}, стоимость ${cost}`;
  $('#trace-take-detail').textContent = `Объём ${volume + item.volume}, стоимость ${cost + item.value}`;
  $('#trace-skip').className = 'trace-choice';
  $('#trace-take').className = 'trace-choice';
  if (branchEvent) $(step.take ? '#trace-take' : '#trace-skip').classList.add(step.type === 'duplicate' ? 'rejected-choice' : 'active-choice');
  if (step.type === 'capacity') $('#trace-take').classList.add('rejected-choice');
}

function renderTraceTree(step, follow) {
  const data = traceView.data;
  const visible = !!step && state.items.length <= 6 && !data.truncated;
  $('#trace-tree-section').hidden = !visible;
  if (!visible || state.view !== 'trace') return;
  const nodes = new Map();
  let bestId = 0;
  for (let i = 0; i <= traceView.index; ++i) {
    const event = data.steps[i];
    if (event.type === 'capacity') {
      const parent = nodes.get(event.node);
      const item = state.items[event.item];
      if (parent && item) nodes.set(`capacity-${event.number}`, {
        id: `capacity-${event.number}`, parent: event.node, path: [...parent.path, 1],
        cost: event.cost + item.value, volume: event.volume + item.volume, cut: true, capacity: true
      });
      continue;
    }
    if (!nodes.has(event.node)) {
      const parent = nodes.get(event.parent);
      nodes.set(event.node, { id: event.node, parent: event.parent, path: parent ? [...parent.path, event.take ? 1 : 0] : [], cost: event.cost, volume: event.volume });
    }
    const node = nodes.get(event.node);
    if (['bound', 'duplicate', 'stale'].includes(event.type)) node.cut = true;
    if (event.type === 'visit') node.visited = true;
    if (event.type === 'leaf') node.leaf = true;
    if (event.type === 'best' || event.type === 'finish') bestId = event.node;
  }
  const width = Math.max(500, 2 ** state.items.length * 50);
  const height = Math.max(110, (state.items.length + 1) * 56 + 28);
  for (const node of nodes.values()) {
    let position = 0;
    for (const direction of node.path) position = position * 2 + direction;
    node.x = (position + .5) * width / 2 ** node.path.length;
    node.y = 25 + node.path.length * 56;
  }
  let lines = '';
  let circles = '';
  for (const node of nodes.values()) {
    const parent = nodes.get(node.parent);
    if (parent) {
      lines += `<path d="M${parent.x} ${parent.y + 13} L${node.x} ${node.y - 13}" class="trace-edge ${node.capacity ? 'capacity-edge' : ''}"/>`;
      if (node.path.length <= 3) lines += `<text x="${(node.x + parent.x) / 2}" y="${(node.y + parent.y) / 2 - 3}" class="trace-edge-label">${node.path.at(-1) ? '+' : '−'}</text>`;
    }
    const label = node.capacity ? '×' : node.cost.toLocaleString('ru-RU', { notation: 'compact', maximumFractionDigits: 1 });
    const classes = ['trace-node', node.cut ? 'cut' : '', node.visited ? 'visited' : '', node.id === bestId ? 'best' : '', node.id === step.node ? 'current' : ''].join(' ');
    circles += `<g class="${classes}" transform="translate(${node.x},${node.y})"><title>${escapeHTML(`${node.capacity ? 'Предмет не помещается' : `Состояние № ${node.id}`}: объём ${node.volume}, стоимость ${node.cost}`)}</title><rect x="-21" y="-13" width="42" height="26" rx="6"/><text text-anchor="middle" dominant-baseline="central">${escapeHTML(label)}</text></g>`;
  }
  $('#trace-tree').innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}" role="img" aria-label="Созданные состояния поиска и отклонённые ветви">${lines}${circles}</svg>`;
  if (follow) {
    const active = nodes.get(step.node);
    const scroll = $('#trace-tree');
    if (active) {
      scroll.scrollLeft = Math.max(0, active.x - (scroll.clientWidth || 600) / 2);
      scroll.scrollTop = Math.max(0, active.y - (scroll.clientHeight || 180) / 2);
    }
  }
}

function renderTrace(follow = false) {
  const data = traceView.data;
  const step = currentTraceStep();
  const optimized = (data?.method || state.method) !== 'brute';
  const atEnd = !!data && traceView.index >= data.steps.length - 1;
  $('#trace-method').textContent = optimized ? 'Оптимизированный поиск' : 'Полный перебор';
  $('#trace-record').disabled = state.busy;
  $('#trace-record').textContent = state.busy && state.view === 'trace' ? 'Записываем…' : data ? 'Записать заново' : 'Записать ход';
  $('#trace-play').disabled = state.busy || !data;
  $('#trace-play').innerHTML = `${icon(traceView.playing ? 'pause' : 'play')}<span>${traceView.playing ? 'Пауза' : atEnd ? 'Повторить' : 'Воспроизвести'}</span>`;
  $('#trace-back').disabled = state.busy || !data || traceView.index === 0 || traceView.playing;
  $('#trace-next').disabled = state.busy || !data || atEnd || traceView.playing;
  $('#trace-end').disabled = state.busy || !data || atEnd;
  $('#trace-position').disabled = state.busy || !data;
  $('#trace-position').max = data ? data.steps.length - 1 : 0;
  $('#trace-position').value = traceView.index;
  $('#trace-speed').disabled = state.busy;
  $('#trace-position-label').textContent = data ? `${traceView.index + 1} / ${data.steps.length}` : 'Нет записи';
  const [title, description] = step ? traceDescription(step) : ['Начните запись', 'Нажмите «Записать ход». Для знакомства подойдёт учебный набор из четырёх предметов.'];
  $('#trace-event-kind').textContent = title;
  $('#trace-event-kind').className = step ? `trace-kind kind-${step.type}` : 'trace-kind';
  $('#trace-description').textContent = description;
  $('#trace-node-label').textContent = step ? `№ ${step.node} · рассмотрено ${step.index}/${state.items.length}` : '';
  $('#trace-volume').textContent = step ? `${step.volume} / ${state.capacity}` : '—';
  $('#trace-cost').textContent = step ? step.cost.toLocaleString('ru-RU') : '—';
  $('#trace-bound').textContent = step?.bound !== null && step?.bound !== undefined ? step.bound.toLocaleString('ru-RU') : '—';
  $('#trace-bound').title = optimized ? 'Текущая стоимость плюс сумма модулей стоимости оставшихся предметов' : 'Полный перебор не использует верхнюю оценку';
  $('#trace-best-cost').textContent = step ? step.bestCost.toLocaleString('ru-RU') : '—';
  $('#trace-visited').textContent = step ? step.visited.toLocaleString('ru-RU') : '0';
  $('#trace-pruned').textContent = step ? step.pruned.toLocaleString('ru-RU') : '0';
  $('#trace-frontier-label').textContent = optimized ? 'В очереди' : 'Глубина';
  $('#trace-pending').textContent = step ? optimized ? step.pending.toLocaleString('ru-RU') : step.index : '0';
  $('#trace-path-items').innerHTML = step?.path.length ? step.path.map(index => {
    const item = state.items[index];
    return `<span class="trace-item" title="Объём ${item.volume}, стоимость ${item.value}">${illustration(icons[item.type])}${escapeHTML(item.name)}</span>`;
  }).join('') : '<span class="trace-empty">Пустой набор</span>';
  $('#trace-choices').hidden = !step;
  if (step) renderTraceChoices(step);
  renderTraceTree(step, follow);
  let note = 'События записаны во время выполнения C++ алгоритма. Замеры времени выполняются отдельно, без записи шагов.';
  if (data?.truncated) note = `Сохранены первые ${data.limit} событий и итог. Всего событий: ${data.totalEvents.toLocaleString('ru-RU')}. На последнем шаге показаны полные счётчики и окончательный ответ.`;
  else if (step && state.items.length > 6) note = 'Для наборов больше 6 предметов показываем состояния и счётчики. Дерево удобно смотреть на учебном наборе.';
  $('#trace-note').textContent = note;
  $('#trace-result').hidden = !atEnd;
  if (atEnd) $('#trace-result-description').textContent = `Итог: стоимость ${data.result.cost}, объём ${data.result.volume} / ${state.capacity}.`;
}

function bindTraceControls() {
  $('#trace-record').addEventListener('click', recordTrace);
  $('#trace-play').addEventListener('click', () => traceView.playing ? pauseTrace() : playTrace());
  $('#trace-back').addEventListener('click', () => { pauseTrace(); seekTrace(traceView.index - 1); });
  $('#trace-next').addEventListener('click', () => { pauseTrace(); seekTrace(traceView.index + 1); });
  $('#trace-end').addEventListener('click', () => { pauseTrace(); if (traceView.data) seekTrace(traceView.data.steps.length - 1); });
  $('#trace-position').addEventListener('input', event => { pauseTrace(); seekTrace(Number(event.target.value)); });
  $('#trace-speed').addEventListener('change', () => { if (traceView.playing) { clearTimeout(traceView.timer); scheduleTraceStep(); } });
  $('#trace-show-result').addEventListener('click', () => {
    if (!traceView.data) return;
    const result = traceView.data.result;
    const method = traceView.data.method;
    showView('packing');
    state.method = method;
    beginAssembly(result, true);
  });
}
