/* ==========================================================================
   Employee Management System - Vanilla JavaScript Client Logic
   ========================================================================== */

// Helper: Toast Notifications
function showToast(message, type = 'info') {
  let container = document.getElementById('toast-container');
  if (!container) {
    container = document.createElement('div');
    container.id = 'toast-container';
    container.className = 'toast-container';
    document.body.appendChild(container);
  }

  const toast = document.createElement('div');
  toast.className = `toast toast-${type === 'error' ? 'error' : 'success'}`;
  
  const icon = type === 'error' ? 'fa-exclamation-circle' : 'fa-check-circle';
  toast.innerHTML = `<i class="fas ${icon}"></i> <span>${message}</span>`;

  container.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateX(100%)';
    toast.style.transition = 'all 0.3s ease';
    setTimeout(() => toast.remove(), 300);
  }, 3500);
}

// Modal Helpers
function openModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) modal.classList.add('active');
}

function closeModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) modal.classList.remove('active');
}

// Role Tab Switching for Dashboard Panels
function switchTab(tabId, btn) {
  const tabContents = document.querySelectorAll('.tab-content');
  tabContents.forEach(tc => tc.classList.remove('active'));

  const tabBtns = document.querySelectorAll('.tab-btn');
  tabBtns.forEach(tb => tb.classList.remove('active'));

  const targetContent = document.getElementById(tabId);
  if (targetContent) targetContent.classList.add('active');
  if (btn) btn.classList.add('active');
}

// Live Updates Polling for Admin and Manager
let livePollInterval = null;

function startLiveUpdatesPoll() {
  const liveBox = document.getElementById('live-updates-list');
  if (!liveBox) return;

  function fetchLogs() {
    fetch('/api/live-updates')
      .then(res => res.json())
      .then(data => {
        if (data.success && data.logs) {
          liveBox.innerHTML = '';
          data.logs.forEach(log => {
            const item = document.createElement('div');
            item.className = 'live-item';
            item.innerHTML = `
              <div>
                <span class="user">[${log.user_role}] ${log.username}</span>
                <span class="action">${log.action} ${log.details ? '- ' + log.details : ''}</span>
              </div>
              <span class="time">${log.time_ago}</span>
            `;
            liveBox.appendChild(item);
          });
        }
      })
      .catch(err => console.error("Live updates poll error:", err));
  }

  fetchLogs();
  livePollInterval = setInterval(fetchLogs, 3000); // 3-second live update refresh
}

// --------------------------------------------------------------------------
// Login & Security Reset Logic
// --------------------------------------------------------------------------
let currentLoginRole = 1; // Default Admin

function selectLoginRole(roleNum) {
  currentLoginRole = roleNum;
  document.querySelectorAll('.role-tab-btn').forEach(btn => btn.classList.remove('active'));
  
  const selectedBtn = document.getElementById(`role-tab-${roleNum}`);
  if (selectedBtn) selectedBtn.classList.add('active');

  const usernameLabel = document.getElementById('username-label');
  const usernameInput = document.getElementById('username-input');
  const portalTitle = document.getElementById('portal-title');

  if (roleNum === 1) {
    if (portalTitle) portalTitle.innerText = "Admin Portal Login";
    if (usernameLabel) usernameLabel.innerText = "Admin Username";
    if (usernameInput) usernameInput.placeholder = "Enter admin username";
  } else if (roleNum === 2) {
    if (portalTitle) portalTitle.innerText = "Manager Portal Login";
    if (usernameLabel) usernameLabel.innerText = "Manager Username";
    if (usernameInput) usernameInput.placeholder = "Enter manager username";
  } else if (roleNum === 3) {
    if (portalTitle) portalTitle.innerText = "Employee Portal Login";
    if (usernameLabel) usernameLabel.innerText = "Employee ID";
    if (usernameInput) usernameInput.placeholder = "Enter Employee ID (e.g. 1001)";
  }
}

function handleLoginSubmit(e) {
  e.preventDefault();
  const username = document.getElementById('username-input').value.trim();
  const password = document.getElementById('password-input').value.trim();

  if (!username || !password) {
    showToast("Please enter username/ID and password.", "error");
    return;
  }

  fetch('/api/login', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      role: currentLoginRole,
      username: username,
      password: password
    })
  })
  .then(res => res.json())
  .then(data => {
    if (data.success) {
      showToast("Login successful! Redirecting...", "success");
      setTimeout(() => { window.location.href = data.redirect; }, 600);
    } else {
      showToast(data.message, "error");
      if (data.lockout) {
        // Trigger Forgot Password Lockout Modal
        const lockId = document.getElementById('reset-identifier');
        if (lockId) lockId.value = username;
        
        const resetLabel = document.getElementById('reset-verify-label');
        const resetInput = document.getElementById('reset-verify-input');
        if (currentLoginRole === 3) {
          if (resetLabel) resetLabel.innerText = "Date of Birth (DD/MM/YYYY)";
          if (resetInput) resetInput.placeholder = "DD/MM/YYYY";
        } else {
          if (resetLabel) resetLabel.innerText = "Verify Email Address";
          if (resetInput) resetInput.placeholder = "Enter account email";
        }
        
        openModal('modal-lockout-reset');
      }
    }
  })
  .catch(err => {
    showToast("Network error. Please try again.", "error");
  });
}

function handlePasswordReset(e) {
  e.preventDefault();
  const identifier = document.getElementById('reset-identifier').value.trim();
  const verifyVal = document.getElementById('reset-verify-input').value.trim();
  const newPassword = document.getElementById('reset-new-password').value.trim();

  fetch('/api/reset-password', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      role: currentLoginRole,
      identifier: identifier,
      verify_value: verifyVal,
      new_password: newPassword
    })
  })
  .then(res => res.json())
  .then(data => {
    if (data.success) {
      showToast(data.message, "success");
      closeModal('modal-lockout-reset');
      document.getElementById('password-input').value = '';
    } else {
      showToast(data.message, "error");
    }
  });
}

// Initializer on Page Load
document.addEventListener('DOMContentLoaded', () => {
  startLiveUpdatesPoll();
});
