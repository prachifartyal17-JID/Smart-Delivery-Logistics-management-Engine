// Owner: Prachi - Track panel (hash table lookup) and the Parcels table.
function renderParcels(s) {
  const body = $("parcel-rows");
  body.replaceChildren();
  s.parcels.forEach(p => {
    const tr = el("tr");
    const status = el("td");
    status.append(el("span", p.status, "badge " + p.status.toLowerCase().replace(/ /g, "-")));
    tr.append(el("td", p.id), el("td", p.dest), status);
    body.append(tr);
  });
}

function setupTrack() {
  $("btn-track").onclick = async () => {
    const out = $("track-result");
    out.replaceChildren();
    try {
      const p = await call("track", { id: $("track-id").value.trim() });
      [["ID", p.id], ["To", p.dest + " (" + p.pincode + ")"], ["Service", p.service], ["Priority", p.priority], ["Status", p.status], ["Charge", money(p.cost)]]
        .forEach(([k, v]) => {
          const row = el("div", undefined, "kv");
          row.append(el("span", k, "muted"), el("span", String(v)));
          out.append(row);
        });
    } catch (e) { say(e.message, true); }
  };
}
