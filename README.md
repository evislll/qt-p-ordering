# PizzaGo — Pizza ordering app

Two versions of the same app:

| Folder | What it is | Where it runs |
|--------|-----------|---------------|
| `qt/`  | The Qt (C++) desktop app — your original project, redesigned | Windows / macOS / Linux |
| `web/` | A browser version (one `index.html`, no build step) | **GitHub Pages**, any browser, phones |

GitHub Pages only serves static files, so it cannot run a compiled Qt desktop program.
That is why `web/` exists: it has the same screens and features and is what you host.

## 1. Run the Qt app

1. Copy your existing `logos.png`, `phut.png`, `phwings.png` into `qt/images/`.
2. Open `qt/pizza_ordering.pro` in **Qt Creator** (Qt 6 or 5.15) → *Configure Project* → press **Run** (Ctrl+R).
   Or from a terminal: `cd qt && qmake && make && ./pizza_ordering` (`qmake6` on some Linux installs).
3. The window opens maximized. **F11** toggles true full screen, **Esc** leaves it.

Only the Qt `widgets` module is needed — the map is drawn with QPainter, so no WebEngine and no internet are required.

Demo accounts are stored locally (QSettings) with a salted SHA-256 hash. A real app needs a server.

## 2. Host the web version on GitHub Pages

1. Create a new GitHub repository (e.g. `pizzago`) and push this whole folder to the `main` branch:
   ```
   git init
   git add .
   git commit -m "PizzaGo"
   git branch -M main
   git remote add origin https://github.com/<your-username>/pizzago.git
   git push -u origin main
   ```
2. On GitHub: **Settings → Pages → Build and deployment → Source: GitHub Actions**.
3. Open the **Actions** tab. The workflow *Deploy web version to GitHub Pages* runs on every push.
   When it is green your site is live at `https://<your-username>.github.io/pizzago/`.

(No Actions? Alternative: Settings → Pages → *Deploy from a branch* → `main` → folder `/web` is not offered, so move `web/index.html` to the repo root or a `/docs` folder and pick that.)

## 3. Optional: run the real Qt app in the browser (WebAssembly)

Qt can compile to WebAssembly, so the C++ app itself can be hosted on Pages too:

1. Install Qt 6.5+ with the **WebAssembly (single-thread)** kit from the Qt Maintenance Tool, plus Emscripten (the Qt docs list the exact version for your Qt).
2. In Qt Creator choose the *WebAssembly* kit, build in Release.
3. Copy the output (`pizza_ordering.html/.js/.wasm`, `qtloader.js`, `qtlogo.svg`) into a folder, rename the html to `index.html`, and deploy that folder the same way as above.
Use the **single-thread** kit: GitHub Pages cannot send the cross-origin headers that multi-thread builds need.
Downsides: a large download (about 20–30 MB) and slower startup than the HTML version, so the HTML version is the better default for sharing.

## Promo codes to try
`PIZZA10` (10% off), `SAVE5` ($5 off), `FREEDEL` (free delivery)
