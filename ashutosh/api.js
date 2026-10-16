// Owner: Ashutosh - shared helpers and the ONE door to the backend.
// Opened as a file (double-click)  -> mock mode: the UI runs with fake data, no C++ needed.
// Opened from the C++ server (http://localhost:3000) -> real mode: every call goes to POST /api/<name>.
const USE_MOCK = location.protocol === "file:";
const BELT_SIZE = 5, VAN_SIZE = 5;

const $ = id => document.getElementById(id);
const money = n => "Rs " + n.toFixed(2);
function el(tag, text, cls) {
  const e = document.createElement(tag);
  if (text !== undefined) e.textContent = text;
  if (cls) e.className = cls;
  return e;
}
function say(text, isError) { const m = $("msg"); m.textContent = text; m.className = isError ? "error" : ""; }
async function act(fn, okText) {
  try { const r = await fn(); if (okText) say(okText(r)); await refresh(); }
  catch (e) { say(e.message, true); }
}

async function call(name, body = {}) {
  if (USE_MOCK) return mock[name](body);
  const res = await fetch("/api/" + name, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body) });
  const data = await res.json();
  if (!res.ok) throw new Error(data.error || "Request failed");
  return data;
}

// ---------- Mock backend (sample data; mirrors what the C++ engine will return) ----------
const PLACES = { "248001": "Clock Tower", "248002": "Clement Town", "248009": "Rajpur Road", "248140": "Jakhan" };
const RATES = {
  standard: { name: "Standard Ground", prio: 2, cost: (w, d) => 40 + w * 10 + d * 1.5 },
  express:  { name: "Prime Express",   prio: 1, cost: (w, d) => 100 + w * 22 + d * 3.5 }
};
const db = { parcels: [], belt: [], van: [], route: ["Central Warehouse"], seq: 0 };
const byId = id => db.parcels.find(p => p.id === id);

const mock = {
  suggest({ prefix }) {                                   // C++ side: Trie prefix search
    const q = prefix.toLowerCase();
    if (!q) return [];
    return Object.entries(PLACES).filter(([pin, name]) => pin.startsWith(q) || name.toLowerCase().startsWith(q)).map(([pin, name]) => ({ pin, name }));
  },
  quote({ service, weight, distance }) { return { cost: RATES[service].cost(weight, distance) }; },   // Strategy pattern
  register({ pincode, service, weight, distance }) {      // hash table insert + heap insert
    if (!PLACES[pincode]) throw new Error("Unknown pincode. Pick one from the suggestions.");
    if (!(weight > 0) || !(distance > 0)) throw new Error("Weight and distance must be positive.");
    const r = RATES[service], seq = ++db.seq;
    const p = { id: String(100000000100 + seq), pincode, dest: PLACES[pincode], service: r.name, priority: r.prio, seq, cost: r.cost(weight, distance), status: "Placed" };
    db.parcels.push(p);
    if (!db.route.includes(p.dest)) db.route.push(p.dest);
    return p;
  },
  track({ id }) {                                         // hash table lookup, O(1)
    const p = byId(id);
    if (!p) throw new Error("No parcel with that ID.");
    return p;
  },
  state() { return JSON.parse(JSON.stringify(db)); },
  dispatch() {                                            // heap extract-min -> circular queue
    if (db.belt.length >= BELT_SIZE) throw new Error("Belt is full.");
    const waiting = db.parcels.filter(p => p.status === "Placed").sort((a, b) => a.priority - b.priority || a.seq - b.seq);
    if (!waiting.length) throw new Error("No parcels waiting.");
    waiting[0].status = "Sorted";
    db.belt.push(waiting[0].id);
    return waiting[0];
  },
  loadVan() {                                             // queue dequeue -> re-stack by priority -> stack push
    if (!db.belt.length) throw new Error("Belt is empty.");
    if (db.van.length >= VAN_SIZE) throw new Error("Van is full.");
    const loaded = db.belt.splice(0, VAN_SIZE - db.van.length);
    // re-stack: rank parcels from earlier loads together with the new ones (priority, then arrival order)
    const cargo = [...db.van, ...loaded].map(byId)
      .sort((x, y) => x.priority - y.priority || x.seq - y.seq);
    db.van.length = 0;
    // stack is last-in-first-out, so push in reverse: most urgent ends on top
    [...cargo].reverse().forEach(par => { db.van.push(par.id); par.status = "In transit"; });
    return { count: loaded.length, top: cargo[0].id };
  },
  deliver() {                                             // stack pop, State pattern -> Delivered
    if (!db.van.length) throw new Error("Van is empty.");
    const id = db.van.pop();
    byId(id).status = "Delivered";
    return { id };
  },
  cancelStop({ name }) {                                  // doubly linked list unlink
    const i = db.route.indexOf(name);
    if (i < 1) throw new Error("That stop can't be cancelled.");
    db.route.splice(i, 1);
    return { name };
  }
};
if (USE_MOCK) [["248001", "standard", 3, 8.5], ["248009", "express", 1.2, 5]].forEach(([pincode, service, weight, distance]) => mock.register({ pincode, service, weight, distance }));
