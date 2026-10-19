# Workload-Aware Adaptive Page Replacement

## Project Overview
This project simulates and compares page replacement algorithms used in operating systems. It includes a C++ simulator and an interactive web interface for visualizing page replacement behavior.

## Algorithms
- **FIFO (First-In, First-Out):** Replaces the page that entered memory first.
- **LRU (Least Recently Used):** Replaces the page that has not been used for the longest time.
- **Optimal:** Replaces the page whose next use is farthest in the future.
- **Adaptive:** Selects between replacement strategies based on workload behavior.

## Technologies Used
- C++
- HTML
- CSS
- JavaScript

## Features
- Accepts a page reference sequence and frame count.
- Compares page faults and page hits.
- Displays hit and fault ratios.
- Provides charts and step-by-step simulation results.

## Project Components

This project consists of two complementary components that work together to analyze and demonstrate workload-aware adaptive page replacement.

### 1. C++ Simulator (VS Code)

The C++ implementation is used to execute and analyze the page replacement algorithms. It allows users to test different reference strings and frame sizes, compare page faults and hits, examine hit and fault ratios, and observe how the Adaptive algorithm behaves under different workloads.

**Algorithms implemented:**
- FIFO (First-In, First-Out)
- LRU (Least Recently Used)
- Optimal Page Replacement
- Adaptive Page Replacement (FIFO/LRU)

### 2. Web-Based Visualization Interface

The web interface provides a more interactive and presentation-friendly way to demonstrate the project. It displays algorithm comparisons, performance parameters, and visual representations of page replacement behavior, making the results easier to understand.

The interface is designed to support project demonstrations by presenting the algorithm behavior and comparison results in a clear, visual format.

### Why Both Components?

The C++ simulator focuses on algorithm execution, testing, and performance analysis, while the web interface focuses on visualization and presentation. Together, they provide both a technical evaluation environment and an accessible way to demonstrate the project.

**Technologies Used:** C++, HTML, CSS, JavaScript, VS Code

## How to Run

### C++ Simulator
Compile the source file using a C++ compiler:

```bash
g++ main_back.cpp -o page_replacement
```

Run on Linux:
```bash
./page_replacement
```

Run on Windows:
```bash
page_replacement.exe
```

### Website
Open `index.html` in a web browser. Keep `index.html`, `style.css`, and `script.js` in the same folder.

## Project Files
- `main_back.cpp` — C++ simulator.
- `index.html` — Website structure.
- `style.css` — Website styling.
- `script.js` — Website logic.
- `Workload_Aware_Adaptive_Page_Replacement.pdf` — Project report.

## Conclusion
This project provides a practical way to understand and compare page replacement algorithms through simulation and visualization.
