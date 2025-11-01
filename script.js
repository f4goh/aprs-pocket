document.addEventListener("DOMContentLoaded", () => {
  const minuteSelect = document.getElementById("minute");
  for (let i = 0; i <= 5; i++) {
    const option = document.createElement("option");
    option.value = i;
    option.textContent = i;
    minuteSelect.appendChild(option);
  }

  const secondSelect = document.getElementById("second");
  for (let i = 0; i < 60; i += 5) {
    const option = document.createElement("option");
    option.value = i;
    option.textContent = i;
    secondSelect.appendChild(option);
  }

  const mobilitySelect = document.getElementById("mobility");
  const mobilityOptions = [
    { value: 0, label: "RUNNER" },
    { value: 1, label: "BIKE" },
    { value: 2, label: "CAR" }
  ];
  mobilityOptions.forEach(opt => {
    const option = document.createElement("option");
    option.value = opt.value;
    option.textContent = opt.label;
    mobilitySelect.appendChild(option);
  });

  const status = document.getElementById("status");
  const infoBloc = document.getElementById("infoBloc");
  const connectBtn = document.getElementById("connectBtn");
  const sendBtn = document.getElementById("sendBtn");

  let port = null;
  sendBtn.disabled = true;

  async function ensurePortOpen() {
    if (!port) {
      port = await navigator.serial.requestPort();
    }

    if (!port.readable || !port.writable) {
      try {
        await port.open({ baudRate: 9600 });
      } catch (err) {
        if (!err.message.includes("already open")) {
          throw err;
        }
      }
    }
  }

  async function sendData(buffer) {
    await ensurePortOpen();

    const writer = port.writable.getWriter();
    await writer.write(new TextEncoder().encode("#"));
    await writer.write(buffer);
    writer.releaseLock();

    status.textContent = "✅ Données envoyées avec succès.";
  }

  async function readSerialData() {
    await ensurePortOpen();
    if (!port || !port.readable) return;
    const decoder = new TextDecoder();
    const reader = port.readable.getReader();

    try {
      let buffer = "";
      while (true) {
        const { value } = await reader.read();
        if (value) {
          buffer += decoder.decode(value, { stream: true });

         // Traite toutes les lignes complètes
        let lines = buffer.split("\r\n");
        buffer = lines.pop(); // garde la dernière ligne incomplète

        for (const line of lines) {
          const pre = document.createElement("pre");
          pre.textContent = line;
          pre.style.margin = "2px 0";
          pre.style.fontSize = "14px";
          pre.style.whiteSpace = "pre-wrap";
          infoBloc.appendChild(pre);
           // Scroll automatique vers le bas
          //infoBloc.scrollTop = infoBloc.scrollHeight;
        }
        }
      }
    } catch (err) {
      console.error("❌ Erreur lecture série :", err);
    } 
  }

  connectBtn.addEventListener("click", async () => {
    if (navigator.userAgent.includes("Firefox")) {
      status.textContent = "Navigateur non compatible avec la connexion série.";
      return;
    }

    try {
      await ensurePortOpen();
      status.textContent = "✅ Connecté au port série.";
      readSerialData(); // démarrer la lecture continue
      sendBtn.disabled = false;     
    } catch (err) {
      status.textContent = "❌ Erreur connexion série : " + err.message;
    }
  });

  sendBtn.addEventListener("click", async () => {
    try {
      infoBloc.innerHTML = ""; // 🧹 nettoie le bloc avant envoi
      const callsignInput = document.getElementById("callsign").value.toUpperCase();
      const callsign = callsignInput.padEnd(9, ' ') + '\0';
      const commentInput = document.getElementById("comment").value;
      const comment = commentInput.padEnd(30, ' ') + '\0';
      const symbolInput = document.getElementById("symbol").value;
      const symbol = symbolInput.length > 0 ? symbolInput.charCodeAt(0) : 0;
      const minute = parseInt(document.getElementById("minute").value);
      const second = parseInt(document.getElementById("second").value);

      if (minute === 0 && second === 0) {
        alert("⛔ Configuration impossible : minute et seconde ne peuvent pas être toutes les deux à zéro.");
        status.textContent = "❌ Envoi annulé : minute et seconde à zéro.";
        return;
      }

      const smart = document.getElementById("smart").checked ? 1 : 0;
      const compressed = document.getElementById("compressed").checked ? 1 : 0;
      const altitude = document.getElementById("altitude").checked ? 1 : 0;
      const display = document.getElementById("display").checked ? 1 : 0;
      const mobility = parseInt(document.getElementById("mobility").value);

      const encoder = new TextEncoder();
      const buffer = new Uint8Array(49);
      let offset = 0;

      encoder.encodeInto(callsign, buffer.subarray(offset, offset + 10));
      offset += 10;
      encoder.encodeInto(comment, buffer.subarray(offset, offset + 31));
      offset += 31;
      buffer[offset++] = symbol;
      buffer[offset++] = minute;
      buffer[offset++] = second;
      buffer[offset++] = smart;
      buffer[offset++] = compressed;
      buffer[offset++] = altitude;
      buffer[offset++] = display;
      buffer[offset++] = mobility;

      await sendData(buffer);      
    } catch (err) {
      status.textContent = "❌ Erreur série : " + err.message;
    }
  });
});

