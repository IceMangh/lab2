const structureLab = {
  structure: 'array', type: 'int', mode: 'manual', operations: [],
  data: null, busy: false, playing: false, timer: null
};

const structureMethods = {
  array: [['resize', 'Resize · изменить размер', ['size']], ['set', 'Set · записать значение', ['index', 'value']], ['get', 'Get · прочитать', ['index']], ['copy', 'Проверить независимость копии', ['value']]],
  tree: [['insert', 'Insert · добавить', ['value']], ['contains', 'Contains · найти', ['value']], ['remove', 'Remove · удалить', ['value']], ['top', 'GetMax · максимум', []], ['pop', 'ExtractMax · извлечь максимум', []], ['height', 'GetHeight · высота', []], ['inorder', 'LKP · симметричный обход', []], ['threads', 'LKPThreads · по прошивке', []], ['reverse', 'PKLThreads · обратный обход', []]],
  hash: [['add', 'Add · добавить пару', ['key', 'value']], ['update', 'Update · заменить значение', ['key', 'value']], ['get', 'Get · найти по ключу', ['key']], ['contains', 'ContainsKey · есть ли ключ', ['key']], ['remove', 'Remove · удалить', ['key']], ['capacity', 'GetCapacity · вместимость', []]],
  queue: [['push', 'Push · добавить', ['value', 'priority']], ['top', 'Top · первый элемент', []], ['pop', 'Pop · извлечь', []], ['empty', 'IsEmpty · пуста ли очередь', []]],
  heap: [['push', 'Push · добавить', ['value']], ['top', 'Top · первый элемент', []], ['pop', 'Pop · извлечь', []], ['empty', 'IsEmpty · пуста ли куча', []]]
};

function labMethods() {
  return structureMethods[structureLab.structure] || structureMethods.heap;
}

function labDisplay(value) {
  if (value === '') return '""';
  return String(value);
}

function pauseStructureLab() {
  clearTimeout(structureLab.timer);
  structureLab.playing = false;
  renderStructureLab();
}

function renderLabFields() {
  const fields = labMethods().find(method => method[0] === $('#structure-operation').value)?.[2] || [];
  for (const name of ['size', 'index', 'key', 'value', 'priority']) {
    const input = $(`#structure-${name}`);
    input.closest('label').hidden = !fields.includes(name);
    input.disabled = structureLab.busy || state.busy || structureLab.mode === 'tests' || !fields.includes(name);
  }
}

function renderStructureLab() {
  const lab = structureLab;
  const data = lab.data;
  const testing = lab.mode === 'tests';
  const busy = lab.busy || state.busy;
  const heap = lab.structure.endsWith('heap');
  const type = lab.type === 'string' ? 'std::string' : lab.type;
  const titles = { array: `DynamicArray<${type}>`, tree: `BinarySearchTree<${type}>`, hash: `HashTable<${type}, ${type}>`, queue: `PriorityQueue<${type}>`, minheap: 'BinaryHeap · минимум', maxheap: 'BinaryHeap · максимум' };
  const hints = {
    array: 'Сначала задайте размер через Resize, затем запишите элементы через Set. Индексы начинаются с нуля. Максимум в этой панели — 128 элементов.',
    tree: 'Дерево балансируется после вставки и удаления. Повторная вставка возвращает false. Можно сравнить обычный обход и обход по прошивке.',
    hash: 'Ключи и значения имеют выбранный тип. У разных ключей может совпасть корзина. При росте или сжатии таблицы индексы корзин пересчитываются.',
    queue: 'Первым извлекается значение с большим приоритетом. При равных приоритетах в твоей реализации первым выйдет добавленный последним.',
    minheap: 'В корне — минимальное значение. BinaryHeap в проекте хранит double, поэтому выбор типа для кучи закреплён.',
    maxheap: 'В корне — максимальное значение. BinaryHeap в проекте хранит double, поэтому выбор типа для кучи закреплён.'
  };
  $('#structure-title').textContent = titles[lab.structure];
  $('#structure-hint').textContent = hints[lab.structure];
  $('#structure-kind').disabled = busy;
  $('#structure-type').disabled = busy || heap;
  $('#structure-tests').disabled = busy;
  $('#structure-reset').disabled = busy;
  $('#structure-operation').disabled = busy || testing;
  $('#structure-apply').disabled = busy || testing || lab.operations.length >= 100;
  $('#structure-form').hidden = testing;
  $('#structure-manual-note').hidden = !testing;
  $('#structure-playback').hidden = !testing;
  $('#structure-play').disabled = state.busy || lab.busy && !lab.playing || !data || !data.total;
  $('#structure-back').disabled = busy || !data?.count;
  $('#structure-next').disabled = busy || !data || data.count >= data.total;
  $('#structure-end').disabled = busy || !data || data.count >= data.total;
  $('#structure-play').innerHTML = `${icon(lab.playing ? 'pause' : 'play')}<span>${lab.playing ? 'Пауза' : data && data.count === data.total ? 'Повторить' : 'Пройти все'}</span>`;
  $('#structure-log-title').textContent = testing ? 'Готовые проверки' : 'История операций';
  $('#structure-check-count').textContent = data ? testing ? `${data.passed} ✓ · ${data.failed} ошибок · ${data.count} / ${data.total}` : `${data.count} / 100` : '';
  renderLabFields();

  const snapshot = data?.snapshot;
  $('#structure-stats').innerHTML = (snapshot?.stats || []).map(stat => `<span>${escapeHTML(stat.label)} <strong>${escapeHTML(stat.value)}</strong></span>`).join('');
  $('#structure-snapshot-title').textContent = snapshot?.title || (lab.structure === 'hash' ? 'Пары по корзинам' : 'Элементы');
  if (lab.structure === 'hash' && snapshot) {
    const capacity = snapshot.stats.find(stat => stat.label === 'Корзин').value;
    const buckets = Array.from({ length: capacity }, (_, index) => {
      const entries = snapshot.entries.filter(entry => entry.bucket === index);
      return `<div class="lab-bucket ${entries.length ? 'occupied' : ''}"><span>${index}</span><div>${entries.length ? entries.map(entry => `<code>${escapeHTML(labDisplay(entry.key))} → ${escapeHTML(labDisplay(entry.value))}</code>`).join('') : '<span>пусто</span>'}</div></div>`;
    });
    $('#structure-snapshot').innerHTML = `<div class="lab-buckets">${buckets.join('')}</div>`;
  } else {
    $('#structure-snapshot').innerHTML = snapshot?.values.length ? `<div class="lab-values">${snapshot.values.map((value, index) => `<div class="lab-cell"><code>${escapeHTML(labDisplay(value))}</code><span>${lab.structure === 'array' ? `[${index}]` : index === 0 && lab.structure !== 'tree' ? 'первый' : index + 1}</span></div>`).join('')}</div>` : '<p class="lab-empty">Пока пусто</p>';
  }

  const steps = data?.steps || [];
  const latest = steps.at(-1);
  $('#structure-result').className = `structure-result ${latest?.passed === false || latest?.error && latest.passed !== true ? 'lab-failed' : ''}`;
  $('#structure-result').textContent = latest ? `${latest.call} → ${labDisplay(latest.actual)}${latest.passed === true ? ' · проверка пройдена' : ''}${latest.detail ? ` · ${latest.detail}` : ''}` : testing ? 'Нажмите «Шаг» или «Пройти все». После каждого шага видно состояние структуры.' : 'Выберите метод и выполните операцию.';
  $('#structure-log').innerHTML = (data?.plan || []).map((call, index) => {
    const step = steps[index];
    const expected = data.expectations[index];
    const failed = step && (step.passed === false || step.error && step.passed !== true);
    const status = !step ? '·' : failed ? '×' : step.passed === true ? '✓' : '→';
    return `<button type="button" class="lab-log-row ${index === steps.length - 1 ? 'current' : ''} ${failed ? 'lab-failed' : ''} ${!step ? 'pending' : ''}" data-lab-step="${index}" ${busy || !testing ? 'disabled' : ''}>
      <span class="lab-call"><span class="lab-status">${status}</span><code>${escapeHTML(call)}</code></span>
      <code>${expected != null ? escapeHTML(labDisplay(expected)) : '—'}</code>
      <code>${step ? escapeHTML(labDisplay(step.actual)) : '—'}</code></button>`;
  }).join('') || '<p class="lab-empty">Операций ещё нет</p>';
}

async function requestStructureLab(count) {
  const lab = structureLab;
  if (lab.busy || state.busy) return false;
  lab.busy = true;
  renderStructureLab();
  try {
    const response = await requestBackend('structure', {
      structure: lab.structure, valueType: lab.type, mode: lab.mode,
      operations: lab.operations, ...(count === undefined ? {} : { count })
    });
    lab.data = response.lab;
    return true;
  } catch (error) {
    lab.playing = false;
    toast(error.message);
    return false;
  } finally {
    lab.busy = false;
    renderStructureLab();
  }
}

function resetStructureLab() {
  pauseStructureLab();
  const lab = structureLab;
  lab.structure = $('#structure-kind').value;
  lab.type = lab.structure.endsWith('heap') ? 'double' : $('#structure-type').value;
  $('#structure-type').value = lab.type;
  lab.operations = [];
  lab.data = null;
  lab.mode = 'manual';
  $('#structure-operation').innerHTML = labMethods().map(([value, title]) => `<option value="${value}">${escapeHTML(title)}</option>`).join('');
  $('#structure-value').value = $('#structure-key').value = lab.type === 'string' ? 'apple' : lab.type === 'double' ? '1.25' : '2';
  $('#structure-size').value = '3';
  $('#structure-index').value = '0';
  $('#structure-priority').value = '1';
  renderStructureLab();
}

async function seekStructureLab(count) {
  pauseStructureLab();
  await requestStructureLab(count);
}

async function playStructureLab() {
  if (structureLab.playing) { pauseStructureLab(); return; }
  if (!structureLab.data || structureLab.busy) return;
  if (structureLab.data.count === structureLab.data.total && !await requestStructureLab(0)) return;
  structureLab.playing = true;
  renderStructureLab();
  const advance = async () => {
    if (!structureLab.playing || state.view !== 'structures') return;
    if (!await requestStructureLab(structureLab.data.count + 1)) return;
    if (structureLab.data.count >= structureLab.data.total) { pauseStructureLab(); return; }
    if (structureLab.playing) structureLab.timer = setTimeout(advance, 500);
  };
  await advance();
}

function structureCSV() {
  const lab = structureLab;
  const rows = [['Структура', lab.structure, 'Тип', lab.type], ['Операция', 'Ожидание', 'Результат', 'Проверка', 'Исключение']];
  for (const step of lab.data?.steps || []) rows.push([step.call, step.expected ?? '', step.actual, step.passed == null ? '' : step.passed ? 'пройдена' : 'ошибка', step.detail]);
  return '\uFEFF' + rows.map(row => row.map(value => `"${String(value).replaceAll('"', '""')}"`).join(',')).join('\r\n');
}

function bindStructureControls() {
  $('#structure-kind').addEventListener('change', resetStructureLab);
  $('#structure-type').addEventListener('change', resetStructureLab);
  $('#structure-operation').addEventListener('change', renderLabFields);
  $('#structure-reset').addEventListener('click', resetStructureLab);
  $('#structure-form').addEventListener('submit', async event => {
    event.preventDefault();
    if (structureLab.busy || state.busy || structureLab.mode !== 'manual') return;
    if (structureLab.operations.length >= 100) { toast('В истории уже 100 операций. Нажмите «Очистить».'); return; }
    const operation = { op: $('#structure-operation').value };
    const fields = labMethods().find(method => method[0] === operation.op)[2];
    for (const name of fields) operation[name] = $(`#structure-${name}`).value;
    structureLab.operations.push(operation);
    if (!await requestStructureLab()) {
      structureLab.operations.pop();
      renderStructureLab();
    }
  });
  $('#structure-tests').addEventListener('click', async () => {
    pauseStructureLab();
    const previous = { mode: structureLab.mode, operations: structureLab.operations, data: structureLab.data };
    structureLab.mode = 'tests';
    structureLab.operations = [];
    structureLab.data = null;
    if (!await requestStructureLab(0)) {
      Object.assign(structureLab, previous);
      renderStructureLab();
    }
  });
  $('#structure-play').addEventListener('click', playStructureLab);
  $('#structure-back').addEventListener('click', () => seekStructureLab(structureLab.data.count - 1));
  $('#structure-next').addEventListener('click', () => seekStructureLab(structureLab.data.count + 1));
  $('#structure-end').addEventListener('click', () => seekStructureLab(structureLab.data.total));
  $('#structure-log').addEventListener('click', event => {
    const row = event.target.closest('[data-lab-step]');
    if (row && structureLab.mode === 'tests' && !structureLab.busy) seekStructureLab(Number(row.dataset.labStep) + 1);
  });
  resetStructureLab();
}
