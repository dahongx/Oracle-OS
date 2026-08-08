const state = {
  view: "home",
  selectedMode: 0,
  holdStart: 0,
  holdFrame: null,
  longPressTriggered: false,
};

const modes = [
  { label: "QUICK ASK", icon: "?" },
  { label: "ORBIT", icon: "◉" },
  { label: "TRIAD", icon: "△" },
  { label: "MOOD", icon: "◇" },
  { label: "DAILY", icon: "☼" },
  { label: "VAULT", icon: "▤" },
];

const results = [
  ["TRUST", "THE FLOW"],
  ["STAY", "WITH THE SIGNAL"],
  ["MOVE", "WITHOUT HURRY"],
  ["REST", "BEFORE THE NEXT TURN"],
  ["NOTICE", "THE QUIET PATTERN"],
  ["KEEP", "WHAT FEELS TRUE"],
];

const views = {
  home: document.querySelector("#homeView"),
  mode: document.querySelector("#modeView"),
  reveal: document.querySelector("#revealView"),
};

const screen = document.querySelector("#screen");
const wheel = document.querySelector("#wheel");
const catButton = document.querySelector("#catButton");
const modeList = document.querySelector("#modeList");
const wheelHint = document.querySelector("#wheelHint");
const holdProgress = document.querySelector("#holdProgress");

function setView(name) {
  state.view = name;
  Object.entries(views).forEach(([key, element]) => {
    element.classList.toggle("view--active", key === name);
  });
}

function renderModes() {
  modeList.innerHTML = "";
  modes.forEach((mode, index) => {
    const button = document.createElement("button");
    button.className = "mode-item" + (index === state.selectedMode ? " is-selected" : "");
    button.innerHTML = `
      <span class="mode-item__icon">${mode.icon}</span>
      <span>${mode.label}</span>
    `;
    button.addEventListener("click", () => {
      state.selectedMode = index;
      renderModes();
      openReveal();
    });
    modeList.appendChild(button);
  });
}

function goHome() {
  setView("home");
}

function openModes() {
  renderModes();
  setView("mode");
}

function openReveal() {
  const [title, body] = results[state.selectedMode];
  document.querySelector("#resultTitle").textContent = title;
  document.querySelector("#resultBody").textContent = body;
  setView("reveal");
}

function rotate(delta) {
  wheelHint.classList.add("is-active");
  clearTimeout(rotate.timer);
  rotate.timer = setTimeout(() => wheelHint.classList.remove("is-active"), 130);

  if (state.view === "home") {
    openModes();
    return;
  }

  if (state.view === "mode") {
    const direction = delta > 0 ? 1 : -1;
    state.selectedMode = (state.selectedMode + direction + modes.length) % modes.length;
    renderModes();
  }
}

function shortPress() {
  if (state.view === "home") openModes();
  else if (state.view === "mode") openReveal();
  else if (state.view === "reveal") {
    screen.animate(
      [
        { boxShadow: "inset 0 0 0 1px rgba(111,103,89,.25)" },
        { boxShadow: "inset 0 0 0 3px rgba(126,132,115,.72)" },
        { boxShadow: "inset 0 0 0 1px rgba(111,103,89,.25)" },
      ],
      { duration: 180, easing: "ease-out" }
    );
  }
}

function longPress() {
  state.longPressTriggered = true;
  goHome();
}

function beginHold() {
  if (state.holdStart) return;
  state.holdStart = performance.now();
  state.longPressTriggered = false;
  holdProgress.classList.add("is-visible");

  const tick = (now) => {
    const progress = Math.min((now - state.holdStart) / 1200, 1);
    holdProgress.style.setProperty("--hold", `${progress * 360}deg`);

    if (progress >= 1) {
      longPress();
      endHold();
      return;
    }
    state.holdFrame = requestAnimationFrame(tick);
  };

  state.holdFrame = requestAnimationFrame(tick);
}

function endHold() {
  if (!state.holdStart) return;

  const elapsed = performance.now() - state.holdStart;
  cancelAnimationFrame(state.holdFrame);
  state.holdStart = 0;
  state.holdFrame = null;
  holdProgress.classList.remove("is-visible");
  holdProgress.style.setProperty("--hold", "0deg");

  if (!state.longPressTriggered && elapsed < 700) shortPress();
}

wheel.addEventListener("wheel", (event) => {
  event.preventDefault();
  rotate(event.deltaY);
}, { passive: false });

screen.addEventListener("wheel", (event) => {
  event.preventDefault();
  rotate(event.deltaY);
}, { passive: false });

wheel.addEventListener("pointerdown", (event) => {
  wheel.setPointerCapture(event.pointerId);
  beginHold();
});

wheel.addEventListener("pointerup", endHold);
wheel.addEventListener("pointercancel", endHold);
catButton.addEventListener("click", openModes);

window.addEventListener("keydown", (event) => {
  if (event.key === "ArrowDown") rotate(1);
  if (event.key === "ArrowUp") rotate(-1);
  if (event.key === "Enter" || event.key === " ") shortPress();
  if (event.key === "Escape") goHome();
});

function updateClock() {
  const now = new Date();
  document.querySelector("#clock").textContent =
    `${String(now.getHours()).padStart(2, "0")}:${String(now.getMinutes()).padStart(2, "0")}`;
}

updateClock();
setInterval(updateClock, 30_000);
renderModes();
