document.addEventListener("DOMContentLoaded", function () {
  const socket = new WebSocket(`ws://${location.hostname}/config`);
  const statusElement = document.getElementById("status");
  const spinner = document.getElementById("spinner");
  const scanBtn = document.getElementById("scanBtn");

  scanBtn.addEventListener("click", requestAvailableNetworks);

  function updateStatus(message, type = 'info') {
    statusElement.textContent = message;
    statusElement.classList.remove('success', 'error');

    if (type === 'success') {
      statusElement.classList.add('success');
    } else if (type === 'error') {
      statusElement.classList.add('error');
    }

    statusElement.classList.remove("fade-out");
    statusElement.classList.add("fade-in");

    setTimeout(() => {
      statusElement.classList.remove("fade-in");
      statusElement.classList.add("fade-out");
    }, 3000);
  }

  function showSpinner() {
    spinner.style.display = "block";
  }

  function hideSpinner() {
    spinner.style.display = "none";
  }

  socket.onopen = () => {
    console.log("Connected to WebSocket server");
    updateStatus("Connected to WebSocket server", 'success');
  };

  socket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    if (message.type === "networkList") {
      updateStatus("Network list received");
      populateNetworkList(message.networks);
      hideSpinner();
    } else if (message.type === "connectionStatus") {
      updateStatus(
        `Connection status: ${message.status} - ip address: ${message.ip}`,
        message.status === 'connected' ? 'success' : 'error'
      );
      hideSpinner();
    }
  };

  socket.onerror = (error) => {
    console.error("WebSocket error:", error);
    updateStatus("WebSocket error occurred", 'error');
  };

  socket.onclose = () => {
    console.log("Disconnected from WebSocket server");
    updateStatus("Disconnected from WebSocket server", 'error');
  };

  function requestAvailableNetworks() {
    console.log("Requesting available networks...");
    updateStatus("Scanning for available networks...");
    showSpinner();
    socket.send(JSON.stringify({ type: "scanNetworks" }));
  }

  // Update the SVG_ICONS with new signal strength icons
  const SVG_ICONS = {
    lockClosed: `<svg viewBox="0 0 24 24">
        <path d="M12,17A2,2 0 0,0 14,15C14,13.89 13.1,13 12,13A2,2 0 0,0 10,15A2,2 0 0,0 12,17M18,8A2,2 0 0,1 20,10V20A2,2 0 0,1 18,22H6A2,2 0 0,1 4,20V10C4,8.89 4.9,8 6,8H7V6A5,5 0 0,1 12,1A5,5 0 0,1 17,6V8H18M12,3A3,3 0 0,0 9,6V8H15V6A3,3 0 0,0 12,3Z"/>
    </svg>`,
    lockOpen: `<svg viewBox="0 0 24 24">
        <path d="M18,8A2,2 0 0,1 20,10V20A2,2 0 0,1 18,22H6C4.89,22 4,21.1 4,20V10A2,2 0 0,1 6,8H15V6A3,3 0 0,0 12,3A3,3 0 0,0 9,6H7A5,5 0 0,1 12,1A5,5 0 0,1 17,6V8H18M12,17A2,2 0 0,0 14,15A2,2 0 0,0 12,13A2,2 0 0,0 10,15A2,2 0 0,0 12,17Z"/>
    </svg>`,
    wifiStrong: `<svg viewBox="0 0 24 24">
        <path d="M12,3C7.79,3 3.7,4.41 0.38,7C4.41,12.06 7.89,16.37 12,21.5C16.08,16.42 20.24,11.24 23.65,7C20.32,4.41 16.22,3 12,3Z"/>
    </svg>`,
    wifiMedium: `<svg viewBox="0 0 24 24">
        <path d="M12,3C7.79,3 3.7,4.41 0.38,7C4.41,12.06 7.89,16.37 12,21.5C16.08,16.42 20.24,11.24 23.65,7C20.32,4.41 16.22,3 12,3M12,5C15.07,5 18.09,5.86 20.71,7.45L12,18.3L3.27,7.44C5.9,5.85 8.92,5 12,5Z"/>
    </svg>`,
    wifiWeak: `<svg viewBox="0 0 24 24">
        <path d="M12,3C7.79,3 3.7,4.41 0.38,7C4.41,12.06 7.89,16.37 12,21.5C16.08,16.42 20.24,11.24 23.65,7C20.32,4.41 16.22,3 12,3M12,5C15.07,5 18.09,5.86 20.71,7.45L12,18.3L3.27,7.44C5.9,5.85 8.92,5 12,5M12,7C14.07,7 16.09,7.86 17.71,9.45L12,16.3L6.27,9.44C7.9,7.85 9.92,7 12,7Z"/>
    </svg>`
  };

  function populateNetworkList(networks) {
    const availableNetworks = document.getElementById('available-networks');
    availableNetworks.innerHTML = '';

    // Deduplicate networks and keep strongest signal
    const uniqueNetworks = networks.reduce((acc, network) => {
      if (!acc[network.ssid] || network.rssi > acc[network.ssid].rssi) {
        acc[network.ssid] = network;
      }
      return acc;
    }, {});

    Object.values(uniqueNetworks).forEach((network, index) => {
      const networkRow = document.createElement('div');
      networkRow.className = 'network-row';

      const radio = document.createElement('input');
      radio.type = 'radio';
      radio.name = 'network';
      radio.value = network.ssid;
      radio.id = `network-${index}`;

      const networkInfo = document.createElement('div');
      networkInfo.className = 'network-info';

      const label = document.createElement('label');
      label.htmlFor = `network-${index}`;
      label.textContent = network.ssid;

      const icons = document.createElement('div');
      icons.className = 'network-icons';

      // Add signal strength icon first
      const signalIcon = document.createElement('div');
      const rssi = network.rssi;
      // Convert RSSI to percentage (typical RSSI range is -100 to -50)
      const signalPercentage = Math.min(100, Math.max(0, (rssi + 100) * 2));

      if (signalPercentage >= 66.667) {
        signalIcon.innerHTML = SVG_ICONS.wifiStrong;
      } else if (signalPercentage >= 33.334) {
        signalIcon.innerHTML = SVG_ICONS.wifiMedium;
      } else {
        signalIcon.innerHTML = SVG_ICONS.wifiWeak;
      }
      signalIcon.title = `Signal Strength: ${rssi} dBm (${Math.round(signalPercentage)}%)`;

      // Add security icon second
      const securityIcon = document.createElement('div');
      securityIcon.innerHTML = network.encryption ? SVG_ICONS.lockClosed : SVG_ICONS.lockOpen;
      securityIcon.title = network.encryption ? 'Secured' : 'Open';

      // Append in new order: signal then lock
      icons.appendChild(signalIcon);
      icons.appendChild(securityIcon);

      networkInfo.appendChild(label);
      networkInfo.appendChild(icons);

      networkRow.appendChild(radio);
      networkRow.appendChild(networkInfo);

      radio.addEventListener('change', () => {
        if (network.encryption) {
          showPasswordField();
        } else {
          hidePasswordField();
        }
      });

      availableNetworks.appendChild(networkRow);
    });

    updateStatus("Select a network to connect.");
  }

  function showPasswordField() {
    const availableNetworks = document.getElementById('available-networks');
    let passwordField = document.getElementById('password-field');
    if (!passwordField) {
      passwordField = document.createElement('div');
      passwordField.id = 'password-field';
      passwordField.innerHTML = `
            <input type="password" class="form-control" id="wifi-password" placeholder="Enter Wi-Fi password">
            <button id="connectBtn" class="square-btn">Connect</button>
        `;
      availableNetworks.appendChild(passwordField);

      document.getElementById('connectBtn').addEventListener('click', connectToNetwork);
    }
  }

  function hidePasswordField() {
    const passwordField = document.getElementById("password-field");
    if (passwordField) passwordField.remove();
  }

  function connectToNetwork() {
    const selectedNetwork = document.querySelector(
      'input[name="network"]:checked'
    ).value;
    const password = document.getElementById("wifi-password").value;
    if (!selectedNetwork || !password) {
      alert("Please select a network and enter the password.");
      return;
    }
    console.log(
      `Attempting to connect to ${selectedNetwork} with password ${password}`
    );
    updateStatus(`Connecting to ${selectedNetwork}...`);
    socket.send(
      JSON.stringify({ type: "connect", ssid: selectedNetwork, password })
    );
  }
});
