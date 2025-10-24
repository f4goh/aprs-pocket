document.addEventListener("DOMContentLoaded", () => {
  const minuteSelect = document.getElementById("minute");
  for (let i = 1; i <= 15; i++) {
    const option = document.createElement("option");
    option.value = i;
    option.textContent = i;
    minuteSelect.appendChild(option);
  }

  const form = document.getElementById("aprsForm");
  const status = document.getElementById("status");

  let port = null;

  async function ensurePortOpen() {
    if (!port) {
      port = await navigator.serial.requestPort();
    }

    if (!port.readable || !port.writable) {
      try {
        await port.open({ baudRate: 9600 });
        //await new Promise(resolve => setTimeout(resolve, 2000)); // Attente après reset Arduino
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
    await writer.write(buffer);
    writer.releaseLock();

    status.textContent = "✅ Données envoyées avec succès.";
  }

  form.addEventListener("submit", async (e) => {
    e.preventDefault();

    if (navigator.userAgent.includes("Firefox")) {
      status.textContent = "Navigateur non compatible avec la connexion série.";
      return;
    }

    const callsignInput = document.getElementById("callsign").value.toUpperCase();
    const callsign = callsignInput.padEnd(9, ' ') + '\0'; // 9 + 1 nul
    const commentInput = document.getElementById("comment").value;
    const comment = commentInput.padEnd(30, ' ') + '\0'; // 30 + 1 nul
    const symbolInput = document.getElementById("symbol").value;
    const symbol = symbolInput.length > 0 ? symbolInput.charCodeAt(0) : 0; // 1 octet
    const minute = parseInt(document.getElementById("minute").value);
    const smart = document.getElementById("smart").checked ? 1 : 0;
    const compressed = document.getElementById("compressed").checked ? 1 : 0;

    const encoder = new TextEncoder();
    const buffer = new Uint8Array(45); // 10 + 31 + 1 + 1 + 1 + 1
    let offset = 0;

    encoder.encodeInto(callsign, buffer.subarray(offset, offset + 10));
    offset += 10;
    encoder.encodeInto(comment, buffer.subarray(offset, offset + 31));
    offset += 31;
    buffer[offset++] = symbol;
    buffer[offset++] = minute;
    buffer[offset++] = smart;
    buffer[offset++] = compressed;

    try {
      await sendData(buffer);
    } catch (err) {
      status.textContent = "❌ Erreur série : " + err.message;
    }
  });
});

