Host Simulator (no hardware)

What it does
- Stubs Arduino and TeensyDMX APIs and runs `main_updated.cpp` on your Mac.
- Prints the same diagnostics your sketch emits. Useful to catch syntax/runtime issues in logic.

Build (macOS)
```
clang++ -std=c++17 -Iinclude run.cpp -o sim
./sim | head -50
```

Notes
- This does not simulate DMX timings or shield behavior; it only exercises code flow and prints.
- If you change header paths in the sketch, update the includes in `run.cpp` accordingly.
