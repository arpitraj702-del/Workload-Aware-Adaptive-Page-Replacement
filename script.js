// =============================================================
//  Page Replacement Simulator - all logic lives in this file.
//  Sections: 1) limits  2) simulator  3) algorithms  4) input
//            5) drawing the page  6) start-up
// =============================================================

// ---------- 1) Limits and settings ----------
const MAX_FRAMES = 20;
const MAX_REFS = 200;
const MAX_PAGE = 999;
const WINDOW = 8; // Adaptive looks at the last 8 references (incl. current)

const COLORS = { FIFO: "#3b82f6", LRU: "#4ade80", Optimal: "#a855f7", Adaptive: "#fbbf24" };

let results = [];      // the 4 results of the latest run
let selectedTab = "FIFO";

// ---------- 2) The generic simulator ----------
// Walks through the references one by one and keeps the frames.
// On a fault with full memory it asks choose(...) which slot to replace.
//   onRef (optional) runs for every reference, hit or fault (Adaptive uses it).
function simulate(name, refs, n, choose, onRef) {
  const frames = Array(n).fill(null);   // page in each slot (null = empty)
  const loadTime = Array(n).fill(-1);   // when each page was loaded (FIFO)
  const lastUsed = Array(n).fill(-1);   // when each page was last used (LRU)
  const res = { name, hits: 0, faults: 0, steps: [] };

  refs.forEach((page, i) => {
    if (onRef) onRef(i, page);
    let slot = frames.indexOf(page);
    const hit = slot !== -1;
    let evicted = null, rule = "", note = "";

    if (hit) {
      res.hits++;
    } else {
      res.faults++;
      slot = frames.indexOf(null);                  // use an empty slot first
      if (slot === -1) {                            // memory full: must evict
        const c = choose(i, frames, loadTime, lastUsed);
        slot = c.slot;
        rule = c.rule || "";
        note = c.note || "";
        evicted = frames[slot];
      }
      frames[slot] = page;
      loadTime[slot] = i;
    }
    lastUsed[slot] = i;                             // hit or load = a "use"
    res.steps.push({ page, hit, frames: frames.slice(), evicted, rule, note });
  });
  return res;
}

// index of the smallest number in a list
function argMin(list) {
  let best = 0;
  for (let i = 1; i < list.length; i++) if (list[i] < list[best]) best = i;
  return best;
}

// ---------- 3) The four algorithms ----------
function runFIFO(refs, n) {
  // oldest loaded page = smallest loadTime
  return simulate("FIFO", refs, n, (i, f, load) => ({ slot: argMin(load) }));
}

function runLRU(refs, n) {
  // least recently used page = smallest lastUsed
  return simulate("LRU", refs, n, (i, f, load, used) => ({ slot: argMin(used) }));
}

function runOptimal(refs, n) {
  // Looks into the FUTURE (refs after i). Benchmark only.
  return simulate("Optimal", refs, n, (i, frames) => {
    let slot = 0, farthest = -1;
    frames.forEach((page, s) => {
      const next = refs.indexOf(page, i + 1);        // -1 = never used again
      const distance = next === -1 ? Infinity : next;
      if (distance > farthest) { farthest = distance; slot = s; }
    });
    return { slot };
  });
}

// A small "pretend memory" used by Adaptive. access(page, time) returns
// true if that pretend memory would have had a fault.
function makeShadow(n, isLRU) {
  const pages = [], stamp = [];
  return function access(page, time) {
    const idx = pages.indexOf(page);
    if (idx !== -1) {                                // hit
      if (isLRU) stamp[idx] = time;                  // LRU refreshes on use
      return false;
    }
    if (pages.length < n) { pages.push(page); stamp.push(time); return true; }
    const victim = argMin(stamp);                    // FIFO: load time, LRU: last use
    pages[victim] = page;
    stamp[victim] = time;
    return true;
  };
}

function runAdaptive(refs, n) {
  // Rule: two shadow memories (FIFO and LRU) see every reference so far.
  // At a real replacement, count each shadow's faults in the last WINDOW
  // references (including the current one). Fewer faults wins; tie -> FIFO.
  // Only past/current references are used, never future ones.
  const shadowFIFO = makeShadow(n, false);
  const shadowLRU = makeShadow(n, true);
  const fifoFaults = [], lruFaults = [];             // 1 = fault, per reference

  const record = (i, page) => {
    fifoFaults[i] = shadowFIFO(page, i) ? 1 : 0;
    lruFaults[i] = shadowLRU(page, i) ? 1 : 0;
  };

  const choose = (i, frames, loadTime, lastUsed) => {
    const from = Math.max(0, i - WINDOW + 1);
    let f = 0, l = 0;
    for (let j = from; j <= i; j++) { f += fifoFaults[j]; l += lruFaults[j]; }
    const useLRU = l < f;
    return {
      slot: argMin(useLRU ? lastUsed : loadTime),
      rule: useLRU ? "LRU" : "FIFO",
      note: "last " + (i - from + 1) + " refs: FIFO faults " + f + ", LRU faults " + l
    };
  };
  return simulate("Adaptive", refs, n, choose, record);
}

// ---------- 4) Reading and checking the input ----------
// Returns { refs, n } or { error: "message" }
function readInput() {
  const text = document.getElementById("refs").value.replace(/,/g, " ").trim();
  const frameText = document.getElementById("frames").value.trim();

  if (text === "") return { error: "Please enter a page reference sequence." };
  const tokens = text.split(/\s+/);
  if (tokens.length > MAX_REFS) return { error: "Too many references (maximum " + MAX_REFS + ")." };
  for (const t of tokens) {
    if (!/^\d+$/.test(t)) return { error: "\"" + t + "\" is not a valid page number. Use whole numbers like 7 0 1." };
    if (Number(t) > MAX_PAGE) return { error: "Page " + t + " is too large (maximum " + MAX_PAGE + ")." };
  }
  if (!/^\d+$/.test(frameText)) return { error: "Number of frames must be a whole number." };
  const n = Number(frameText);
  if (n < 1 || n > MAX_FRAMES) return { error: "Number of frames must be between 1 and " + MAX_FRAMES + "." };

  return { refs: tokens.map(Number), n };
}

// ---------- 5) Drawing the page ----------
const pct = (part, total) => (total === 0 ? 0 : (100 * part) / total).toFixed(2) + "%";

function renderSummary(refs, n) {
  const real = results.filter(r => r.name !== "Optimal");        // Optimal is only a benchmark
  const best = Math.min(...real.map(r => r.faults));
  const winners = real.filter(r => r.faults === best).map(r => r.name).join(", ");
  const optimal = results.find(r => r.name === "Optimal");
  const adaptive = results.find(r => r.name === "Adaptive");
  const f = adaptive.steps.filter(s => s.rule === "FIFO").length;
  const l = adaptive.steps.filter(s => s.rule === "LRU").length;

  const rows = [
    ["References", refs.length],
    ["Frames", n],
    ["Fewest faults (FIFO / LRU / Adaptive)", winners + " (" + best + ")"],
    ["Optimal benchmark faults", optimal.faults],
    ["Adaptive replacements", f + " by FIFO, " + l + " by LRU"]
  ];
  document.getElementById("summary").innerHTML =
    rows.map(r => "<div><dt>" + r[0] + "</dt><dd>" + r[1] + "</dd></div>").join("");
}

function renderTable() {
  document.querySelector("#results-table tbody").innerHTML = results.map(r => {
    const total = r.hits + r.faults;
    const tag = r.name === "Optimal" ? '<span class="tag">benchmark</span>' : "";
    return "<tr><td><span class='dot' style='background:" + COLORS[r.name] + "'></span>" + r.name + tag + "</td>" +
      "<td>" + r.faults + "</td><td>" + r.hits + "</td>" +
      "<td>" + pct(r.hits, total) + "</td><td>" + pct(r.faults, total) + "</td></tr>";
  }).join("");
}

function renderChart() {
  const max = Math.max(...results.map(r => r.faults), 1);
  const top = Math.ceil(max / 4) * 4;                 // axis maximum (multiple of 4)
  let grid = "";
  for (let k = 0; k <= 4; k++) {
    const v = (top / 4) * k;
    grid += "<div class='gl' style='bottom:" + (v / top) * 100 + "%'><span>" + v + "</span></div>";
  }
  const bars = results.map(r =>
    "<div class='col'><div class='bar' style='height:" + (r.faults / top) * 100 +
    "%;background:" + COLORS[r.name] + "'><b>" + r.faults + "</b></div></div>").join("");
  const labels = results.map(r => "<span>" + r.name + "</span>").join("");

  document.getElementById("chart").innerHTML =
    "<div class='plot'>" + grid + "<div class='bars'>" + bars + "</div></div><div class='labels'>" + labels + "</div>";
}

function renderTabs() {
  const box = document.getElementById("step-tabs");
  box.innerHTML = results.map(r =>
    "<button data-name='" + r.name + "' class='" + (r.name === selectedTab ? "on" : "") + "'>" + r.name + "</button>").join("");
  box.querySelectorAll("button").forEach(b => b.addEventListener("click", () => {
    selectedTab = b.dataset.name;
    renderTabs();
    renderSteps();
  }));
}

function renderSteps() {
  const r = results.find(x => x.name === selectedTab);
  const isAdaptive = r.name === "Adaptive";
  let html = "<thead><tr><th>#</th><th>Reference</th><th>Frames after reference</th><th>Result</th><th>Evicted</th>" +
             (isAdaptive ? "<th>Strategy used</th>" : "") + "</tr></thead><tbody>";
  r.steps.forEach((s, i) => {
    const cells = s.frames.map(p =>
      "<span class='cell" + (p === s.page ? " cur" : "") + "'>" + (p === null ? "&ndash;" : p) + "</span>").join("");
    html += "<tr><td>" + (i + 1) + "</td><td>" + s.page + "</td><td>" + cells + "</td>" +
      "<td><span class='badge " + (s.hit ? "hit'>HIT" : "miss'>FAULT") + "</span></td>" +
      "<td>" + (s.evicted === null ? "&mdash;" : s.evicted) + "</td>" +
      (isAdaptive ? "<td>" + (s.rule ? "<span class='rule-" + s.rule + "'>" + s.rule + "</span>" : "&mdash;") + "</td>" : "") +
      "</tr>";
  });
  document.getElementById("steps-table").innerHTML = html + "</tbody>";
}

function renderAdaptive() {
  const adaptive = results.find(r => r.name === "Adaptive");
  const list = [];
  adaptive.steps.forEach((s, i) => { if (s.rule) list.push({ s, i }); });

  const f = list.filter(x => x.s.rule === "FIFO").length;
  document.getElementById("adaptive-summary").textContent = list.length === 0
    ? "No replacement was needed, so Adaptive never had to choose a strategy (memory never filled up)."
    : "Adaptive replaced " + list.length + " page(s): " + f + " with FIFO and " + (list.length - f) +
      " with LRU. For each replacement it compared the shadow FIFO and shadow LRU faults over the last " + WINDOW + " references.";

  let html = "";
  if (list.length > 0) {
    html = "<thead><tr><th>Ref #</th><th>Page requested</th><th>Evicted</th><th>Strategy chosen</th><th>Evidence (past references only)</th></tr></thead><tbody>" +
      list.map(x => "<tr><td>" + (x.i + 1) + "</td><td>" + x.s.page + "</td><td>" + x.s.evicted +
        "</td><td><span class='rule-" + x.s.rule + "'>" + x.s.rule + "</span></td><td>" + x.s.note + "</td></tr>").join("") + "</tbody>";
  }
  document.getElementById("adaptive-table").innerHTML = html;
}

// ---------- 6) Start-up ----------
function runSimulation() {
  const input = readInput();
  const error = document.getElementById("error");
  if (input.error) { error.textContent = input.error; return; }
  error.textContent = "";

  results = [
    runFIFO(input.refs, input.n),
    runLRU(input.refs, input.n),
    runOptimal(input.refs, input.n),
    runAdaptive(input.refs, input.n)
  ];
  renderSummary(input.refs, input.n);
  renderTable();
  renderChart();
  renderTabs();
  renderSteps();
  renderAdaptive();
}

document.getElementById("run-btn").addEventListener("click", runSimulation);
document.getElementById("refs").addEventListener("keydown", e => { if (e.key === "Enter") runSimulation(); });
document.getElementById("frames").addEventListener("keydown", e => { if (e.key === "Enter") runSimulation(); });

// picking an example fills the sequence box; typing your own resets the dropdown
document.getElementById("preset").addEventListener("change", e => {
  if (e.target.value) { document.getElementById("refs").value = e.target.value; runSimulation(); }
});
document.getElementById("refs").addEventListener("input", () => {
  document.getElementById("preset").value = "";
});

runSimulation(); // show results for the default example on first load