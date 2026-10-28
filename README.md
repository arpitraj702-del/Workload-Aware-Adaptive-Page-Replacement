# Workload-Aware Adaptive Page Replacement

🌐 **Live Website:** https://workload-aware-page-replacement.netlify.app/

## Project Overview

This project simulates and compares page replacement algorithms used in operating systems. It includes a C++ simulator and an interactive web interface for visualizing page replacement behavior.

The project demonstrates how different page replacement strategies handle memory frames, page requests, page faults, and page hits under different workloads.

## Algorithms

- **FIFO (First-In, First-Out):** Replaces the page that entered memory first.
- **LRU (Least Recently Used):** Replaces the page that has not been used for the longest time.
- **Optimal:** Replaces the page whose next use is farthest in the future.
- **Adaptive:** Selects between replacement strategies based on observed workload behavior.

## Technologies Used

- C++
- HTML
- CSS
- JavaScript
- Visual Studio Code
- Netlify (website deployment)

## Features

- Accepts a page-reference sequence and frame count.
- Supports sequential, repeated, mixed, and custom workloads.
- Compares page faults and page hits.
- Displays hit ratios and fault ratios.
- Provides charts and step-by-step simulation results.
- Shows page replacement decisions and frame updates.
- Supports comparison of all four algorithms.
- Includes a live web interface for demonstrations and visualization.

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

The web interface provides an interactive and presentation-friendly way to demonstrate the project. It displays algorithm comparisons, performance parameters, and visual representations of page replacement behavior, making the results easier to understand.

**Live Demo:** https://workload-aware-page-replacement.netlify.app/

The website is hosted on Netlify and can be opened in a browser without compiling the C++ simulator.

### Why Both Components?

The C++ simulator focuses on algorithm execution, testing, and performance analysis, while the web interface focuses on visualization and presentation. Together, they provide a technical environment for evaluating the algorithms and an accessible interface for demonstrating their behavior.

## How to Run

### C++ Simulator

Make sure a C++ compiler such as GCC is installed.

**Compile the source file:**

```bash
g++ -std=c++11 -Wall -Wextra main_back.cpp -o page_replacement
```

**Run on Linux:**

```bash
./page_replacement
```

**Run on Windows PowerShell:**

```powershell
.\page_replacement.exe
```

If compilation produces `pagesim.exe` instead, run that executable:

```powershell
.\pagesim.exe
```

### Website

You can access the deployed website directly:

https://workload-aware-page-replacement.netlify.app/

To run the website locally, open `index.html` in a web browser. Keep `index.html`, `style.css`, and `script.js` in the same folder.

## Project Files

- `main_back.cpp` — C++ simulator implementation.
- `main.cpp` — Additional C++ source file, if used in the project.
- `index.html` — Website structure.
- `style.css` — Website styling.
- `script.js` — Website logic and simulation behavior.
- `Workload_Aware_Adaptive_Page_Replacement.pdf` — Project report.

## Conclusion

This project provides a practical way to understand and compare page replacement algorithms through simulation and visualization. By implementing FIFO, LRU, Optimal, and Adaptive strategies, it demonstrates how memory-management performance changes with different page-reference patterns. The C++ simulator supports execution and analysis, while the web interface provides an accessible way to visualize and demonstrate the results.

---

**Author:** Arpit Raj
