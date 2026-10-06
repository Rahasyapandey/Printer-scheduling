# PrintPulse - Printer Scheduling System (C core + web UI)

The scheduler is written in C (`printer_scheduler.c`: Queue + Priority Queue/min-heap + fairness rule).
It is compiled to WebAssembly (`scheduler.wasm`), and the web page (`index.html`) calls the C functions directly.

## Run the C program in the terminal
    gcc -Wall -o printer_scheduler printer_scheduler.c && ./printer_scheduler

## Rebuild the WebAssembly core (only if you edit the C code)
    pip install ziglang
    python -m ziglang cc -target wasm32-wasi -Oz -s -mexec-model=reactor -o scheduler.wasm printer_scheduler.c
(Emscripten also works.)

## Test locally (wasm must be served over http, not opened as a file)
    python -m http.server 8000     # then open http://localhost:8000

## Publish (free static hosting)
- Netlify: drag-and-drop this folder at app.netlify.com/drop
- GitHub Pages: push the 3 files to a repo, then Settings > Pages > deploy from main branch
- Vercel: `npx vercel` inside this folder
