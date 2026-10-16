// Owner: Ashutosh - start-up and the Hub overview panel. Loaded last.
function renderOverview(s) {
  const box = $("overview-body");
  box.replaceChildren();
  [["Parcels registered", s.parcels.length], ["On conveyor belt", s.belt.length + " / " + BELT_SIZE], ["In van", s.van.length + " / " + VAN_SIZE]]
    .forEach(([k, v]) => {
      const row = el("div", undefined, "kv");
      row.append(el("span", k, "muted"), el("strong", String(v)));
      box.append(row);
    });
}

async function refresh() {
  const s = await call("state");
  renderOverview(s); renderBelt(s); renderVan(s); renderRoute(s); renderParcels(s);
}

setupRegister(); setupBelt(); setupRoute(); setupTrack();
refresh();
