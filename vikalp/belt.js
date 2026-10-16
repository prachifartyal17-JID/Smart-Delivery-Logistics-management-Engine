// Owner: Vikalp - Conveyor belt (circular queue) and Van (stack) panels.
function renderBelt(s) {
  const bar = $("belt-bar");
  bar.replaceChildren();
  for (let i = 0; i < BELT_SIZE; i++) {
    const id = s.belt[i];
    bar.append(id ? Object.assign(el("div", id.slice(-4), "slot full"), { title: id }) : el("div", "empty", "slot"));
  }
}

function renderVan(s) {
  const box = $("van-body");
  box.replaceChildren();
  if (!s.van.length) box.append(el("p", "Van is empty.", "muted"));
  [...s.van].reverse().forEach((id, i) => box.append(Object.assign(el("div", id + (i === 0 ? "  (top, unloads first)" : ""), "stack-item"), { title: id })));
}

function setupBelt() {
  $("btn-dispatch").onclick = () => act(() => call("dispatch"), p => "Dispatched " + p.id + " (priority " + p.priority + ")");
  $("btn-load").onclick = () => act(() => call("loadVan"), r => "Loaded " + r.count + (r.count === 1 ? " parcel" : " parcels") + ", " + r.top + " is on top");
  $("btn-deliver").onclick = () => act(() => call("deliver"), r => "Delivered " + r.id);
}
