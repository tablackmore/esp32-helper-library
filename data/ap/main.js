document.addEventListener("DOMContentLoaded", function () {
  const socket = new WebSocket(`ws://${location.hostname}/ws`);
  const statusElement = document.getElementById("status");
  const spinner = document.getElementById("spinner");
  const scanBtn = document.getElementById("scanBtn");

  scanBtn.addEventListener("click", requestAvailableNetworks);

  function updateStatus(message, color = "#007BFF") {
    statusElement.textContent = message;
    statusElement.style.color = color;
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
    updateStatus("Connected to WebSocket server", "green");
  };

  socket.onmessage = (event) => {
    const message = JSON.parse(event.data);
    if (message.type === "networkList") {
      updateStatus("Network list received");
      populateNetworkList(message.networks);
      hideSpinner();
    } else if (message.type === "connectionStatus") {
      updateStatus(
        `Connection status: ${message.status} - ip address: ${message.ip} `
      );
      hideSpinner();
    }
  };

  socket.onerror = (error) => {
    console.error("WebSocket error:", error);
    updateStatus("WebSocket error occurred", "red");
  };

  socket.onclose = () => {
    console.log("Disconnected from WebSocket server");
    updateStatus("Disconnected from WebSocket server", "red");
  };

  function requestAvailableNetworks() {
    console.log("Requesting available networks...");
    updateStatus("Scanning for available networks...");
    showSpinner();
    socket.send(JSON.stringify({ type: "scanNetworks" }));
  }

  function populateNetworkList(networks) {
    const availableNetworks = document.getElementById("available-networks");
    availableNetworks.innerHTML = "";

    networks.forEach((network, index) => {
      const networkRow = document.createElement("div");
      networkRow.className = "wifi-row";

      const radio = document.createElement("input");
      radio.type = "radio";
      radio.name = "network";
      radio.value = network.ssid;
      radio.id = `network-${index}`;

      const label = document.createElement("label");
      label.htmlFor = `network-${index}`;
      label.textContent = network.ssid;

      networkRow.appendChild(radio);
      networkRow.appendChild(label);

      radio.addEventListener("change", () => {
        if (network !== "ESP32_Network") {
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
    const availableNetworks = document.getElementById("available-networks");
    let passwordField = document.getElementById("password-field");
    if (!passwordField) {
      passwordField = document.createElement("div");
      passwordField.id = "password-field";
      passwordField.innerHTML = `
          <input type="password" id="wifi-password" placeholder="Enter Wi-Fi password">
          <button id="connectBtn">Connect</button>
        `;
      availableNetworks.appendChild(passwordField);

      document
        .getElementById("connectBtn")
        .addEventListener("click", connectToNetwork);
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
