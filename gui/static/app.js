const START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const PRESETS = {
  start: START_FEN,
  mate6: "r4k2/pp1P2pp/1q6/3Q2p1/2p5/4R3/5PPP/6K1 w - - 0 1",
  kiwipete: "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
};
const PIECES = {
  K: "♔", Q: "♕", R: "♖", B: "♗", N: "♘", P: "♙",
  k: "♚", q: "♛", r: "♜", b: "♝", n: "♞", p: "♟",
};

const state = {
  fen: START_FEN,
  legalMoves: [],
  selected: null,
  orientation: "white",
  history: [],
  moves: [],
  bestMove: null,
  alternatives: [],
  busy: false,
  puzzleBestMove: null,
};

const $ = (selector) => document.querySelector(selector);
const boardEl = $("#board");
const fenInput = $("#fen-input");
const toastEl = $("#toast");

async function api(path, payload = {}) {
  const response = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const data = await response.json();
  if (!response.ok) throw new Error(data.error || "A kérés sikertelen.");
  return data;
}

async function apiGet(path) {
  const response = await fetch(path, { cache: "no-store" });
  const data = await response.json();
  if (!response.ok) throw new Error(data.error || "A kérés sikertelen.");
  return data;
}

function parseFen(fen) {
  const [placement, turn = "w"] = fen.trim().split(/\s+/);
  const pieces = {};
  placement.split("/").forEach((row, rowIndex) => {
    let file = 0;
    for (const token of row) {
      if (/\d/.test(token)) file += Number(token);
      else {
        pieces[`${"abcdefgh"[file]}${8 - rowIndex}`] = token;
        file += 1;
      }
    }
  });
  return { pieces, turn };
}

function squareOrder() {
  const files = state.orientation === "white" ? "abcdefgh" : "hgfedcba";
  const ranks = state.orientation === "white" ? "87654321" : "12345678";
  return [...ranks].flatMap((rank) => [...files].map((file) => `${file}${rank}`));
}

function renderBoard() {
  const { pieces, turn } = parseFen(state.fen);
  const destinations = state.selected
    ? state.legalMoves.filter((move) => move.startsWith(state.selected)).map((move) => move.slice(2, 4))
    : [];
  boardEl.replaceChildren();

  squareOrder().forEach((square) => {
    const fileIndex = square.charCodeAt(0) - 97;
    const rank = Number(square[1]);
    const piece = pieces[square];
    const button = document.createElement("button");
    button.type = "button";
    button.id = `sq-${square}`;
    button.className = `square ${(fileIndex + rank) % 2 ? "light" : "dark"}`;
    button.setAttribute("role", "gridcell");
    button.setAttribute("aria-label", `${square}${piece ? `, ${piece}` : ""}`);
    if (state.selected === square) button.classList.add("selected");
    if (destinations.includes(square)) button.classList.add("legal");
    if (piece) {
      button.classList.add("has-piece");
      const span = document.createElement("span");
      span.className = `piece ${piece === piece.toUpperCase() ? "white" : "black"}`;
      span.textContent = PIECES[piece];
      button.append(span);
    }
    const displayFiles = state.orientation === "white" ? "abcdefgh" : "hgfedcba";
    const displayRanks = state.orientation === "white" ? "87654321" : "12345678";
    if (square[1] === displayRanks[7]) button.append(coord("file", square[0]));
    if (square[0] === displayFiles[0]) button.append(coord("rank", square[1]));
    button.addEventListener("click", () => onSquare(square));
    boardEl.append(button);
  });

  $("#turn-label").textContent = turn === "w" ? "Világos következik" : "Sötét következik";
  $("#legal-count").textContent = `${state.legalMoves.length} legális`;
  $("#position-state").textContent = state.legalMoves.length ? (turn === "w" ? "világos lép" : "sötét lép") : "végállás";
  fenInput.value = state.fen;
}

function coord(kind, text) {
  const span = document.createElement("span");
  span.className = `coord ${kind}`;
  span.textContent = text;
  return span;
}

function onSquare(square) {
  if (state.busy) return;
  if (state.selected) {
    const candidates = state.legalMoves.filter((move) => move.startsWith(state.selected + square));
    if (candidates.length === 1) return playMove(candidates[0]);
    if (candidates.length > 1) return choosePromotion(candidates);
  }
  const canMove = state.legalMoves.some((move) => move.startsWith(square));
  state.selected = canMove ? square : null;
  renderBoard();
}

function choosePromotion(candidates) {
  const dialog = $("#promotion-dialog");
  const options = $("#promotion-options");
  const turn = parseFen(state.fen).turn;
  options.replaceChildren();
  candidates.forEach((move) => {
    const promotion = move[4];
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = PIECES[turn === "w" ? promotion.toUpperCase() : promotion];
    button.addEventListener("click", () => { dialog.close(); playMove(move); });
    options.append(button);
  });
  dialog.showModal();
}

async function loadFen(fen, { resetHistory = true, puzzle = false } = {}) {
  setBusy(true, "FEN betöltése…");
  try {
    const data = await api("/api/state", { fen: fen.trim() });
    state.fen = data.fen;
    state.legalMoves = data.legalMoves;
    state.selected = null;
    state.bestMove = null;
    state.alternatives = [];
    if (!puzzle) {
      state.puzzleBestMove = null;
      $("#puzzle-solution").hidden = true;
      $("#puzzle-source").textContent = "helyi importált állások";
    }
    if (resetHistory) { state.history = []; state.moves = []; }
    clearAnalysis();
    renderAll();
    return true;
  } catch (error) { showToast(error.message, true); return false; }
  finally { setBusy(false); }
}

async function playMove(move) {
  setBusy(true, `${move} kijátszása…`);
  const previousFen = state.fen;
  try {
    const data = await api("/api/move", { fen: state.fen, move });
    if (!data.accepted) throw new Error("A motor elutasította a lépést.");
    state.history.push(previousFen);
    state.moves.push(move);
    state.fen = data.fen;
    state.legalMoves = data.legalMoves;
    state.selected = null;
    state.bestMove = null;
    state.alternatives = [];
    clearAnalysis();
    renderAll();
  } catch (error) { showToast(error.message, true); }
  finally { setBusy(false); }
}

async function analyze({ play = false } = {}) {
  if (state.busy || !state.legalMoves.length) return;
  setBusy(true, "MCTS keresés fut…");
  try {
    const iterations = Number($("#iterations").value);
    const exploration = Number($("#exploration").value);
    const data = await api("/api/analyze", {
      fen: state.fen,
      iterations,
      exploration,
      learnedPolicy: $("#learned-policy").checked,
      policyLearning: $("#policy-learning").checked,
    });
    state.bestMove = data.bestMove === "0000" ? null : data.bestMove;
    state.alternatives = data.alternatives || [];
    showEvaluation(data.scoreCp);
    $("#best-move").textContent = state.bestMove || "—";
    $("#node-count").textContent = Number(data.nodes).toLocaleString("hu-HU");
    $("#play-best-button").disabled = !state.bestMove;
    renderCandidates();
    renderPolicyStatus(data.policy);
    if (play && state.bestMove) await playMove(state.bestMove);
  } catch (error) { showToast(error.message, true); }
  finally { setBusy(false); }
}

function renderPolicyStatus(policy = {}) {
  const status = $("#policy-status");
  if (!policy.trained) {
    status.textContent = `${policy.samples || 0} minta · még nincs tanított modell`;
  } else {
    status.textContent = `${policy.samples || 0} minta · ${policy.updates || 0} tanítás`;
  }
  if (policy.sampleCollected) status.textContent += " · új minta";
  if (policy.trainedNow) status.textContent += " · modell frissítve";
}

async function quickEvaluate() {
  setBusy(true, "Anyagi értékelés…");
  try {
    const data = await api("/api/evaluate", { fen: state.fen });
    showEvaluation(data.scoreCp);
    $("#best-move").textContent = "statikus";
    $("#node-count").textContent = "0";
  } catch (error) { showToast(error.message, true); }
  finally { setBusy(false); }
}

function showEvaluation(cp) {
  const pawns = cp / 100;
  $("#evaluation").textContent = `${pawns >= 0 ? "+" : ""}${pawns.toFixed(2)}`;
  $("#eval-marker").style.left = `${Math.max(4, Math.min(96, 50 + cp / 20))}%`;
}

function clearAnalysis() {
  state.bestMove = null;
  state.alternatives = [];
  $("#best-move").textContent = "—";
  $("#node-count").textContent = "—";
  $("#play-best-button").disabled = true;
  showEvaluation(0);
  renderCandidates();
}

function renderCandidates() {
  const body = $("#candidate-list");
  body.replaceChildren();
  if (!state.alternatives.length) {
    const row = document.createElement("tr");
    row.className = "candidate-empty";
    const cell = document.createElement("td");
    cell.colSpan = 6;
    cell.textContent = "Indíts elemzést a rangsorhoz.";
    row.append(cell);
    body.append(row);
    return;
  }

  state.alternatives.forEach((candidate) => {
    const row = document.createElement("tr");
    const score = candidate.scoreCp / 100;
    const mateLabels = {
      proven: '<span class="proven-mark">kényszermatt</span>',
      refuted: '<span class="proven-mark refuted">védhető</span>',
      unknown: '<span class="proven-mark unknown">nem eldöntött</span>',
    };
    const statusLabel = mateLabels[candidate.mateStatus]
      || (candidate.proven ? '<span class="proven-mark">bizonyított</span>' : '');
    row.innerHTML = `
      <td>${candidate.rank}</td>
      <td><span class="candidate-move">${candidate.move}</span>${statusLabel}</td>
      <td class="candidate-score ${score > 0 ? 'positive' : score < 0 ? 'negative' : ''}">${score >= 0 ? '+' : ''}${score.toFixed(2)}</td>
      <td class="candidate-prior">${Number(candidate.aiPrior || 0).toLocaleString("hu-HU", {minimumFractionDigits: 1, maximumFractionDigits: 1})}%</td>
      <td>${Number(candidate.visits).toLocaleString("hu-HU")}</td>
      <td><button type="button" class="candidate-play" aria-label="${candidate.move} kijátszása">kijátszás</button></td>`;
    row.querySelector("button").addEventListener("click", () => playMove(candidate.move));
    body.append(row);
  });
}

function renderMoves() {
  const list = $("#move-list");
  list.replaceChildren();
  if (!state.moves.length) {
    const item = document.createElement("li");
    item.className = "empty";
    item.textContent = "Még nincs lépés.";
    list.append(item);
    return;
  }
  state.moves.forEach((move) => {
    const item = document.createElement("li");
    item.textContent = move;
    list.append(item);
  });
  list.scrollTop = list.scrollHeight;
}

function renderAll() { renderBoard(); renderMoves(); }

function setBusy(busy, message = "") {
  state.busy = busy;
  ["#analyze-button", "#engine-move-button", "#load-fen-button", "#quick-eval-button", "#random-puzzle-button"].forEach((id) => {
    $(id).disabled = busy;
    $(id).classList.toggle("busy", busy);
  });
  if (message) $("#position-state").textContent = message;
  if (!busy) {
    const turn = parseFen(state.fen).turn;
    $("#position-state").textContent = state.legalMoves.length
      ? (turn === "w" ? "világos lép" : "sötét lép")
      : "végállás";
  }
}

let toastTimer;
function showToast(message, error = false) {
  clearTimeout(toastTimer);
  toastEl.textContent = message;
  toastEl.className = `toast show${error ? " error" : ""}`;
  toastTimer = setTimeout(() => { toastEl.className = "toast"; }, 2600);
}

$("#flip-button").addEventListener("click", () => {
  state.orientation = state.orientation === "white" ? "black" : "white";
  renderBoard();
});
$("#new-button").addEventListener("click", () => loadFen(START_FEN));
$("#load-fen-button").addEventListener("click", () => loadFen(fenInput.value));
$("#analyze-button").addEventListener("click", () => analyze());
$("#engine-move-button").addEventListener("click", () => analyze({ play: true }));
$("#play-best-button").addEventListener("click", () => state.bestMove && playMove(state.bestMove));
$("#quick-eval-button").addEventListener("click", quickEvaluate);
$("#random-puzzle-button").addEventListener("click", async () => {
  if (state.busy) return;
  setBusy(true, "Lichess-feladvány betöltése…");
  try {
    const puzzle = await apiGet("/api/puzzle/random");
    state.puzzleBestMove = puzzle.bestMove;
    $("#puzzle-best-move").textContent = "elrejtve";
    $("#puzzle-source").textContent = `Lichess ${puzzle.puzzleId}`;
    $("#puzzle-solution").hidden = false;
    if (await loadFen(puzzle.fen, { puzzle: true })) {
      showToast("Lichess-feladvány betöltve.");
    }
  } catch (error) { showToast(error.message, true); }
  finally { setBusy(false); }
});
$("#reveal-puzzle-button").addEventListener("click", () => {
  $("#puzzle-best-move").textContent = state.puzzleBestMove || "—";
});
$("#undo-button").addEventListener("click", () => {
  if (!state.history.length || state.busy) return;
  const fen = state.history.pop();
  state.moves.pop();
  loadFen(fen, { resetHistory: false });
});
$("#copy-fen-button").addEventListener("click", async () => {
  await navigator.clipboard.writeText(state.fen);
  showToast("FEN a vágólapra másolva.");
});
$("#iterations").addEventListener("input", (event) => {
  $("#iteration-output").textContent = Number(event.target.value).toLocaleString("hu-HU");
});
$("#exploration").addEventListener("input", (event) => {
  $("#exploration-output").textContent = Number(event.target.value).toLocaleString("hu-HU", {
    minimumFractionDigits: 2,
    maximumFractionDigits: 2,
  });
});
$("#train-policy-button").addEventListener("click", async () => {
  if (state.busy) return;
  setBusy(true, "Policy tanítása…");
  try {
    await api("/api/policy", { action: "train" });
    showToast("A policy tanítása befejeződött.");
    $("#policy-status").textContent = "modell frissítve · elemzéssel ellenőrizhető";
  } catch (error) { showToast(error.message, true); }
  finally { setBusy(false); }
});
$("#reset-policy-button").addEventListener("click", async () => {
  if (state.busy || !window.confirm("Töröljük a tanult modellt és az összes mintát?")) return;
  setBusy(true, "Policy törlése…");
  try {
    await api("/api/policy", { action: "clear" });
    $("#learned-policy").checked = false;
    $("#policy-status").textContent = "0 minta · még nincs tanított modell";
    showToast("A tanult policy törölve.");
  } catch (error) { showToast(error.message, true); }
  finally { setBusy(false); }
});
document.querySelectorAll("[data-preset]").forEach((button) => {
  button.addEventListener("click", () => { fenInput.value = PRESETS[button.dataset.preset]; loadFen(fenInput.value); });
});

fetch("/api/health")
  .then((response) => response.json())
  .then(() => { $("#engine-state").classList.add("online"); $("#engine-state").lastChild.textContent = " motor online"; })
  .catch(() => showToast("A motor nem érhető el.", true));
loadFen(START_FEN);
