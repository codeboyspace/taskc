# Focus & Time-Tracking Toolkit (focusc)

A minimal CLI focus and time-tracking toolkit written in C. Session-based, no external dependencies, pure POSIX terminals.

## Build

```bash
gcc -Wall -Wextra -O2 main.c -o focusc
```

## Usage

```bash
./focusc
```

## Workflow

1. **Start the program** → prompted for task name
2. **Enter task name** → press Enter to start the timer
3. **Timer runs** with live updates every second (HH:MM:SS format)
4. **Control the timer:**
   - Press `Enter` to stop and record the task
   - Press `k` to pause/resume the timer
5. **After stopping:**
   - Task name and duration displayed
   - Choose: Start next task (1) or End session (2)
6. **At session end:**
   - List of all tasks with durations
   - ASCII bar chart showing relative time per task
   - Total session time
   - Average task duration

## Features

- **Non-blocking input**: Terminal doesn't freeze; responds immediately to key presses
- **Live timer display**: Updates every second on the same line using `\r` (carriage return)
- **Pause/resume**: Press `k` at any time to pause or resume the timer
- **Session summary**: ASCII bar chart and statistics at the end
- **In-memory storage**: All data kept in RAM for the session (max 100 tasks)
- **Pure C99 + POSIX**: No external libraries; uses only `<unistd.h>`, `<termios.h>`, `<time.h>`, and standard C

## Technical Details

### Architecture

- **Terminal I/O**: Uses `termios` to enable raw (non-canonical) input mode with non-blocking character reads
- **Timing**: Uses `time()` for elapsed seconds calculation
- **Data structures**: 
  - `Task` struct: stores task name and elapsed seconds
  - `Session` struct: manages array of tasks and total session time
  - Fixed-size array for up to 100 tasks per session

### Key Functions

| Function | Purpose |
|----------|---------|
| `enable_raw_mode()` | Disable canonical input to capture individual keystrokes |
| `get_char_nonblocking()` | Non-blocking key detection using `read()` |
| `run_timer()` | Main loop for timing with pause/resume support |
| `format_time()` | Convert seconds to HH:MM:SS format |
| `print_ascii_chart()` | Render bar chart of task durations |
| `print_session_summary()` | Display final statistics |

### Input Handling

The program uses POSIX `termios` to:
1. Disable canonical mode (`ICANON`)
2. Disable echo (`ECHO`)
3. Set non-blocking reads (`VMIN=0, VTIME=0`)
4. Restore original settings on exit

## Code Organization

```
main.c
├── Terminal I/O Functions (enable_raw_mode, disable_raw_mode, get_char_nonblocking)
├── Input Handling (read_task_name, show_task_menu)
├── Timer Functions (get_current_time, format_time, run_timer)
├── Task Recording (record_task)
├── Session Summary (get_max_elapsed, print_ascii_chart, print_session_summary)
├── Main Session Loop (run_session)
└── Main Entry Point (main)
```

## Example Session

```
=== Focus & Time-Tracking Toolkit ===
Controls: Enter=stop timer, k=pause/resume

Enter task name: Write documentation
Task: Write documentation
Timer: 00:05:23

Task: Write documentation
Time spent: 00:05:23

Menu:
1. Start next task
2. End session
Choice (1-2): 1

Enter task name: Code review
Task: Code review
Timer: 00:03:15

...

=== Session Summary ===

Task Name                            Time      Bar
---                                  ---       
Write documentation                  00:05:23  ########################################
Code review                          00:03:15  ##########################
Meetings                             00:02:00  ################

=== Statistics ===
Total session time: 00:10:38
Average task duration: 00:03:32
Tasks completed: 3
```

## Constraints Met

✓ Language: C99  
✓ Platform: Linux terminal only  
✓ No external libraries (only standard + POSIX)  
✓ No GUI, colors, or networking  
✓ In-memory session data only  
✓ ASCII-only output  
✓ Modular code with clear separation  
✓ Uses `struct` for task storage  
✓ Uses `time()` for timing  
✓ Uses `termios` for non-blocking input  
✓ Clean timer display with `\r` (no scrolling)  
✓ Single `main.c` file  

## Compilation Flags

- `-Wall -Wextra`: Enable all common warnings
- `-O2`: Optimize for performance
- Standard compile target: C99 or later

## Behavior Notes

- Timer precision: 1-second granularity (updates every second)
- Max task name length: 255 characters
- Max tasks per session: 100
- Pause/resume feature: Accumulates paused time correctly
- Terminal restoration: Automatic on program exit, even with Ctrl+C (when possible)

## Known Limitations

- No persistent storage (session data lost on exit)
- Single-threaded; no concurrent task tracking
- No input validation for task names (spaces and special chars OK)
- Bar chart scales to fit terminal width dynamically
- Very long task names may overflow the display (truncated naturally)

## License

Public domain. Use freely.
