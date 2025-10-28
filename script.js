document.addEventListener("DOMContentLoaded", () => {
  const minuteSelect = document.getElementById("minute");
  for (let i = 1; i <= 15; i++) {
    const option = document.createElement("option");
    option.value = i;
    option.textContent = i;
    minuteSelect.appendChild(option);
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

  const form = document.getElementById("aprsForm");
  const status = document.getElementById("status");
  const infoBloc = document.getElementById("infoBloc");

  let port = null;

  async function ensurePortOpen() {
    if (!port) {
      port = await navigator.serial.requestPort(); // déclenché par clic
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
    await writer.write(new TextEncoder().encode("#")); // identifiant
    await writer.write(buffer);                        // 47 octets
    writer.releaseLock();

    status.textContent = "✅ Données envoyées avec succès.";
  }

  async function readSerialData() {
    const decoder = new TextDecoder();
    const reader = port.readable.getReader();

    try {
      let buffer = "";
      while (true) {
        const { value, done } = await reader.read();
        if (done) break;
        if (value) {
          buffer += decoder.decode(value, { stream: true });

          // Regroupe les lignes en un seul bloc
          if (buffer.includes("\r\n")) {
            const lines = buffer.split("\r\n").filter(line => line.trim() !== "");
            buffer = ""; // réinitialise le buffer

            const compactText = lines.join(" "); // concatène avec espace
            const p = document.createElement("p");
            p.textContent = compactText;
            p.style.margin = "2px 0"; // réduit l'espacement vertical
            p.style.fontSize = "14px";
            infoBloc.appendChild(p);
          }
        }
      }
    } catch (err) {
      console.error("❌ Erreur lecture série :", err);
    } finally {
      reader.releaseLock();
    }
  }

  form.addEventListener("submit", async (e) => {
    e.preventDefault();

    if (navigator.userAgent.includes("Firefox")) {
      status.textContent = "Navigateur non compatible avec la connexion série.";
      return;
    }

    try {
      const callsignInput = document.getElementById("callsign").value.toUpperCase();
      const callsign = callsignInput.padEnd(9, ' ') + '\0'; // 10 octets
      const commentInput = document.getElementById("comment").value;
      const comment = commentInput.padEnd(30, ' ') + '\0'; // 31 octets
      const symbolInput = document.getElementById("symbol").value;
      const symbol = symbolInput.length > 0 ? symbolInput.charCodeAt(0) : 0;
      const minute = parseInt(document.getElementById("minute").value);
      const smart = document.getElementById("smart").checked ? 1 : 0;
      const compressed = document.getElementById("compressed").checked ? 1 : 0;
      const altitude = document.getElementById("altitude").checked ? 1 : 0;
      const mobility = parseInt(document.getElementById("mobility").value); // 0, 1 ou 2

      const encoder = new TextEncoder();
      const buffer = new Uint8Array(47); // structure complète
      let offset = 0;

      encoder.encodeInto(callsign, buffer.subarray(offset, offset + 10));
      offset += 10;
      encoder.encodeInto(comment, buffer.subarray(offset, offset + 31));
      offset += 31;
      buffer[offset++] = symbol;
      buffer[offset++] = minute;
      buffer[offset++] = smart;
      buffer[offset++] = compressed;
      buffer[offset++] = altitude;
      buffer[offset++] = mobility;

      await sendData(buffer);
      await readSerialData(); // lecture après envoi
    } catch (err) {
      status.textContent = "❌ Erreur série : " + err.message;
    }
  });
});

