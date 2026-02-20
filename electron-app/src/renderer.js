const statusEl = document.getElementById('status');
const tableBody = document.getElementById('devices-body');
const refreshButton = document.getElementById('refresh');

const formatGB = (bytes) => {
  if (!bytes || Number.isNaN(bytes)) return 'n/a';
  const gb = bytes / (1024 * 1024 * 1024);
  return `${gb.toFixed(2)} GB`;
};

const renderDevices = (devices) => {
  tableBody.innerHTML = '';
  if (!devices.length) {
    statusEl.textContent = 'No devices found.';
    return;
  }
  statusEl.textContent = `${devices.length} device(s) detected.`;
  devices.forEach((d) => {
    const row = document.createElement('tr');
    row.innerHTML = `
      <td>${d.name || 'Unknown'}</td>
      <td>${d.model || 'Unknown'}</td>
      <td>${d.serial || 'Unknown'}</td>
      <td>${d.isMobile ? 'Mobile' : 'Disk'}</td>
      <td>${formatGB(d.capacityBytes)}</td>
      <td>${formatGB(d.freeBytes)}</td>
    `;
    tableBody.appendChild(row);
  });
};

const refresh = async () => {
  statusEl.textContent = 'Scanning…';
  try {
    if (!window.ipod || typeof window.ipod.listDevices !== 'function') {
      statusEl.textContent = 'Bridge unavailable: preload did not expose ipod API.';
      return;
    }
    const devices = await window.ipod.listDevices();
    renderDevices(devices);
  } catch (err) {
    statusEl.textContent = `Error: ${err.message}`;
  }
};

refreshButton.addEventListener('click', refresh);
window.addEventListener('DOMContentLoaded', refresh);
