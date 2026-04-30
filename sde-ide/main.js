let components = [];
let selectedId = null;

const canvas = document.getElementById('canvas');
const propPanel = document.getElementById('prop-controls');
const noSelection = document.getElementById('no-selection');

// UI Elements for properties
const propText = document.getElementById('prop-text');
const propX = document.getElementById('prop-x');
const propY = document.getElementById('prop-y');
const propW = document.getElementById('prop-w');
const propH = document.getElementById('prop-h');
const propColor = document.getElementById('prop-color');
const propAction = document.getElementById('prop-action');

// Drag and Drop from Sidebar
document.querySelectorAll('.block-item').forEach(item => {
  item.addEventListener('dragstart', (e) => {
    e.dataTransfer.setData('type', item.dataset.type);
  });
});

canvas.addEventListener('dragover', (e) => {
  e.preventDefault();
});

canvas.addEventListener('drop', (e) => {
  e.preventDefault();
  const type = e.dataTransfer.getData('type');
  const rect = canvas.getBoundingClientRect();
  const x = Math.round(e.clientX - rect.left);
  const y = Math.round(e.clientY - rect.top);

  addComponent(type, x, y);
});

function addComponent(type, x, y) {
  const id = Date.now().toString();
  const component = {
    id,
    type,
    x,
    y,
    width: type === 'button' ? 120 : (type === 'rect' ? 100 : (type === 'circle' ? 50 : 0)),
    height: type === 'button' ? 40 : (type === 'rect' ? 100 : (type === 'circle' ? 50 : 0)),
    text: type === 'label' ? 'New Label' : (type === 'button' ? 'Click Me' : ''),
    color: '#38bdf8',
    action: ''
  };

  components.push(component);
  renderCanvas();
  selectComponent(id);
}

function renderCanvas() {
  // Clear non-system elements
  const elements = canvas.querySelectorAll('.canvas-element');
  elements.forEach(el => el.remove());

  components.forEach(comp => {
    const el = document.createElement('div');
    el.className = `canvas-element ${comp.id === selectedId ? 'selected' : ''}`;
    el.style.left = `${comp.x}px`;
    el.style.top = `${comp.y}px`;
    el.style.width = comp.width ? `${comp.width}px` : 'auto';
    el.style.height = comp.height ? `${comp.height}px` : 'auto';
    el.dataset.id = comp.id;

    if (comp.type === 'label') {
      el.innerText = comp.text;
      el.style.color = comp.color;
      el.style.fontSize = '16px';
    } else if (comp.type === 'button') {
      el.innerText = comp.text;
      el.style.background = comp.color;
      el.style.color = '#fff';
      el.style.borderRadius = '4px';
      el.style.display = 'flex';
      el.style.alignItems = 'center';
      el.style.justifyContent = 'center';
      el.style.fontWeight = 'bold';
    } else if (comp.type === 'rect') {
      el.style.background = comp.color;
    } else if (comp.type === 'circle') {
      el.style.background = comp.color;
      el.style.borderRadius = '50%';
    }

    el.addEventListener('mousedown', (e) => {
      e.stopPropagation();
      selectComponent(comp.id);
      startDragging(e, comp.id);
    });

    canvas.appendChild(el);
  });
}

function selectComponent(id) {
  selectedId = id;
  const comp = components.find(c => c.id === id);
  
  if (comp) {
    noSelection.style.display = 'none';
    propPanel.style.display = 'flex';
    
    propText.value = comp.text || '';
    propX.value = comp.x;
    propY.value = comp.y;
    propW.value = comp.width || 0;
    propH.value = comp.height || 0;
    propColor.value = comp.color;
    propAction.value = comp.action || '';
    
    // Show/hide relevant fields
    propText.parentElement.style.display = (comp.type === 'label' || comp.type === 'button') ? 'flex' : 'none';
    propW.parentElement.style.display = (comp.type !== 'label') ? 'flex' : 'none';
    propH.parentElement.style.display = (comp.type !== 'label') ? 'flex' : 'none';
    propAction.parentElement.style.display = (comp.type === 'button') ? 'flex' : 'none';
  } else {
    noSelection.style.display = 'block';
    propPanel.style.display = 'none';
  }
  
  renderCanvas();
}

// Update component from properties
[propText, propX, propY, propW, propH, propColor, propAction].forEach(input => {
  input.addEventListener('input', () => {
    if (!selectedId) return;
    const comp = components.find(c => c.id === selectedId);
    if (!comp) return;
    
    comp.text = propText.value;
    comp.x = parseInt(propX.value);
    comp.y = parseInt(propY.value);
    comp.width = parseInt(propW.value);
    comp.height = parseInt(propH.value);
    comp.color = propColor.value;
    comp.action = propAction.value;
    
    renderCanvas();
  });
});

document.getElementById('btn-delete').addEventListener('click', () => {
  if (selectedId) {
    components = components.filter(c => c.id !== selectedId);
    selectedId = null;
    selectComponent(null);
  }
});

canvas.addEventListener('mousedown', () => {
  selectComponent(null);
});

// Dragging on canvas
let isDragging = false;
let dragTargetId = null;
let offset = { x: 0, y: 0 };

function startDragging(e, id) {
  isDragging = true;
  dragTargetId = id;
  const comp = components.find(c => c.id === id);
  const rect = canvas.getBoundingClientRect();
  offset.x = e.clientX - rect.left - comp.x;
  offset.y = e.clientY - rect.top - comp.y;
}

window.addEventListener('mousemove', (e) => {
  if (isDragging && dragTargetId) {
    const comp = components.find(c => c.id === dragTargetId);
    const rect = canvas.getBoundingClientRect();
    comp.x = Math.round(e.clientX - rect.left - offset.x);
    comp.y = Math.round(e.clientY - rect.top - offset.y);
    
    propX.value = comp.x;
    propY.value = comp.y;
    
    renderCanvas();
  }
});

window.addEventListener('mouseup', () => {
  isDragging = false;
  dragTargetId = null;
});

// Export Logic
document.getElementById('btn-export').addEventListener('click', () => {
  const appName = document.getElementById('app-name').value;
  const iconColor = document.getElementById('app-icon-color').value;
  
  const sdeData = {
    version: "1.0",
    app_info: {
      name: appName,
      icon_color: iconColor.replace('#', '0x'),
      description: "Gridz App"
    },
    ui: components.map(c => ({
      type: c.type,
      x: c.x,
      y: c.y,
      w: c.width,
      h: c.height,
      text: c.text,
      color: c.color.replace('#', '0x'),
      action: c.action
    }))
  };

  const blob = new Blob([JSON.stringify(sdeData, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `${appName.toLowerCase().replace(/ /g, '_')}.sde`;
  a.click();
  
  // Confetti effect
  import('https://cdn.skypack.dev/canvas-confetti').then(confetti => {
    confetti.default({
      particleCount: 150,
      spread: 70,
      origin: { y: 0.6 }
    });
  });
});
