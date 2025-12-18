#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/* ANSI Color Codes */
#define COLOR_RESET     "\033[0m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_DIM       "\033[2m"

#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BLACK     "\033[30m"

#define BG_BLUE         "\033[44m"
#define BG_CYAN         "\033[46m"
#define BG_BLACK        "\033[40m"

/* Constants */
#define MAX_TASKS 100
#define MAX_TASK_NAME 256
#define TIMER_REFRESH 1  /* seconds between timer updates */

/* Task structure */
typedef struct {
    char name[MAX_TASK_NAME];
    long elapsed;  /* seconds */
} Task;

/* Session structure */
typedef struct {
    Task tasks[MAX_TASKS];
    int task_count;
    long total_session_time;
} Session;

/* Global state */
static struct termios orig_termios, new_termios;
static Session session = {0};

/* ============================================================================
 * Terminal I/O Functions
 * ============================================================================ */

/* Enable non-canonical, non-blocking input */
void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    new_termios = orig_termios;
    
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;   /* Non-blocking */
    new_termios.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
}

/* Restore original terminal settings */
void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/* Get a single character if available, returns -1 if none */
int get_char_nonblocking(void) {
    unsigned char c;
    ssize_t nread = read(STDIN_FILENO, &c, 1);
    if (nread == 1) {
        return (int)c;
    }
    return -1;
}

/* Clear terminal screen */
void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

/* ============================================================================
 * Banner and UI
 * ============================================================================ */

void print_banner(void) {
    printf("%s%s", COLOR_CYAN, BG_BLACK);
    printf("\n");
    printf("  ███████╗ ██████╗  ██████╗██╗   ██╗███████╗ ██████╗\n");
    printf("  ██╔════╝██╔═══██╗██╔════╝██║   ██║██╔════╝██╔════╝\n");
    printf("  █████╗  ██║   ██║██║     ██║   ██║███████╗██║     \n");
    printf("  ██╔══╝  ██║   ██║██║     ██║   ██║╚════██║██║     \n");
    printf("  ██║     ╚██████╔╝╚██████╗╚██████╔╝███████║╚██████╗\n");
    printf("  ╚═╝      ╚═════╝  ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝\n");
    printf("\n");
    printf("  %s%sFocus & Time-Tracking Toolkit%s\n", COLOR_YELLOW, COLOR_BOLD, COLOR_RESET);
    printf("%s", COLOR_RESET);
}

void print_divider(void) {
    printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", 
           COLOR_CYAN, COLOR_RESET);
}

void print_section_header(const char *title) {
    printf("\n%s◆ %s%s%s\n", COLOR_MAGENTA, COLOR_BOLD, title, COLOR_RESET);
    print_divider();
}

/* ============================================================================
 * Input Handling
 * ============================================================================ */

/* Read task name from user input (blocking) */
void read_task_name(char *name, size_t max_len) {
    printf("%s▶ %sEnter task name:%s ", COLOR_GREEN, COLOR_BOLD, COLOR_RESET);
    fflush(stdout);
    
    if (fgets(name, max_len, stdin) != NULL) {
        /* Remove newline */
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] == '\n') {
            name[len - 1] = '\0';
        }
    }
}

/* Show session menu after a task */
int show_task_menu(void) {
    int choice = -1;
    while (choice < 1 || choice > 2) {
        printf("\n%s━ Menu ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", 
               COLOR_MAGENTA, COLOR_RESET);
        printf("%s1%s Start next task\n", COLOR_YELLOW, COLOR_RESET);
        printf("%s2%s End session\n", COLOR_YELLOW, COLOR_RESET);
        printf("%s▶ Choice (1-2):%s ", COLOR_GREEN, COLOR_RESET);
        fflush(stdout);
        
        int c = getchar();
        if (c == '1') {
            choice = 1;
        } else if (c == '2') {
            choice = 2;
        }
    }
    return choice;
}

/* ============================================================================
 * Timer Functions
 * ============================================================================ */

/* Get current time in seconds since epoch */
long get_current_time(void) {
    return (long)time(NULL);
}

/* Format seconds into HH:MM:SS */
void format_time(long seconds, char *buf, size_t buf_len) {
    long hours = seconds / 3600;
    long minutes = (seconds % 3600) / 60;
    long secs = seconds % 60;
    snprintf(buf, buf_len, "%ld:%02ld:%02ld", hours % 100, minutes, secs);
}

/* Run the timer for a task */
long run_timer(const char *task_name) {
    char time_str[16];
    long elapsed = 0;
    long start_time = get_current_time();
    long paused_time = 0;
    int is_paused = 0;
    
    enable_raw_mode();
    
    printf("\n%s━ Task Started ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", 
           COLOR_CYAN, COLOR_RESET);
    printf("%s▸%s %s\n", COLOR_BLUE, COLOR_RESET, task_name);
    printf("\n%sControls: %s[%sEnter%s] = stop  |  [%sk%s] = pause/resume\n", 
           COLOR_DIM, COLOR_RESET, COLOR_YELLOW, COLOR_RESET, 
           COLOR_YELLOW, COLOR_RESET);
    printf("\n%sTimer: ", COLOR_BOLD);
    fflush(stdout);
    
    while (1) {
        long current_time = get_current_time();
        
        if (is_paused) {
            elapsed = paused_time;
        } else {
            elapsed = paused_time + (current_time - start_time);
        }
        
        format_time(elapsed, time_str, sizeof(time_str));
        
        if (is_paused) {
            printf("\r%sTimer: %s%s %s[PAUSED]%s", 
                   COLOR_BOLD, COLOR_YELLOW, time_str, COLOR_RED, COLOR_RESET);
        } else {
            printf("\r%sTimer: %s%s%s", 
                   COLOR_BOLD, COLOR_GREEN, time_str, COLOR_RESET);
        }
        fflush(stdout);
        
        int key = get_char_nonblocking();
        
        if (key == '\n' || key == '\r') {
            /* Stop timer */
            printf("\n");
            disable_raw_mode();
            return elapsed;
        } else if (key == 'k' || key == 'K') {
            if (is_paused) {
                /* Resume */
                is_paused = 0;
                start_time = current_time;
            } else {
                /* Pause */
                is_paused = 1;
                paused_time = elapsed;
            }
        }
        
        sleep(1);
    }
}

/* ============================================================================
 * Task Recording
 * ============================================================================ */

/* Record a task in the session */
void record_task(const char *name, long elapsed) {
    if (session.task_count < MAX_TASKS) {
        strncpy(session.tasks[session.task_count].name, name, MAX_TASK_NAME - 1);
        session.tasks[session.task_count].name[MAX_TASK_NAME - 1] = '\0';
        session.tasks[session.task_count].elapsed = elapsed;
        session.task_count++;
        session.total_session_time += elapsed;
    }
}

/* ============================================================================
 * Session Summary Display - Graph Chart
 * ============================================================================ */

/* Find the maximum elapsed time for scaling bar chart */
long get_max_elapsed(void) {
    long max_elapsed = 0;
    for (int i = 0; i < session.task_count; i++) {
        if (session.tasks[i].elapsed > max_elapsed) {
            max_elapsed = session.tasks[i].elapsed;
        }
    }
    return max_elapsed > 0 ? max_elapsed : 1;
}

/* Get color based on position for variety */
const char* get_bar_color(int index) {
    const char* colors[] = {COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_MAGENTA, COLOR_YELLOW};
    return colors[index % 5];
}

/* Draw an enhanced ASCII graph chart */
void print_graph_chart(void) {
    long max_elapsed = get_max_elapsed();
    int chart_width = 35;
    
    print_section_header("Session Summary - Time Distribution");
    
    printf("\n%s┌────────────────────────────────────┬──────────┬────────────────────────┐%s\n",
           COLOR_CYAN, COLOR_RESET);
    printf("%s│ %-34s │ %s%-8s%s │ %sDistribution%s           │%s\n",
           COLOR_CYAN, "Task", COLOR_YELLOW, "Time", COLOR_CYAN, COLOR_GREEN, COLOR_RESET, COLOR_CYAN);
    printf("%s├────────────────────────────────────┼──────────┼────────────────────────┤%s\n",
           COLOR_CYAN, COLOR_RESET);
    
    for (int i = 0; i < session.task_count; i++) {
        Task *t = &session.tasks[i];
        char time_str[16];
        format_time(t->elapsed, time_str, sizeof(time_str));
        
        int bar_length = (int)((t->elapsed * chart_width) / max_elapsed);
        if (bar_length == 0 && t->elapsed > 0) {
            bar_length = 1;
        }
        
        const char *color = get_bar_color(i);
        
        /* Truncate task name if too long */
        char task_display[35];
        if (strlen(t->name) > 33) {
            strncpy(task_display, t->name, 30);
            task_display[30] = '\0';
            strcat(task_display, "...");
        } else {
            strcpy(task_display, t->name);
        }
        
        printf("%s│ %-34s │ %s%-8s%s │ %s", 
               COLOR_CYAN, task_display, COLOR_YELLOW, time_str, COLOR_CYAN, color);
        
        for (int j = 0; j < bar_length; j++) {
            printf("█");
        }
        printf("%s│%s\n", COLOR_CYAN, COLOR_RESET);
    }
    
    printf("%s└────────────────────────────────────┴──────────┴────────────────────────┘%s\n",
           COLOR_CYAN, COLOR_RESET);
}

/* Print statistics section */
void print_statistics(void) {
    print_section_header("Statistics");
    
    char total_str[16];
    format_time(session.total_session_time, total_str, sizeof(total_str));
    
    printf("%s  ▸ Total Session Time: %s%s%s\n", 
           COLOR_DIM, COLOR_GREEN, total_str, COLOR_RESET);
    
    if (session.task_count > 0) {
        long avg_time = session.total_session_time / session.task_count;
        char avg_str[16];
        format_time(avg_time, avg_str, sizeof(avg_str));
        printf("%s  ▸ Average Task Duration: %s%s%s\n", 
               COLOR_DIM, COLOR_YELLOW, avg_str, COLOR_RESET);
    }
    
    printf("%s  ▸ Tasks Completed: %s%d%s\n", 
           COLOR_DIM, COLOR_BLUE, session.task_count, COLOR_RESET);
}

/* Print full session summary */
void print_session_summary(void) {
    printf("\n");
    printf("%s%s", BG_CYAN, COLOR_BLACK);
    printf("\n");
    printf("  ╔═══════════════════════════════════════════════════════════╗\n");
    printf("  ║                   SESSION COMPLETE                        ║\n");
    printf("  ╚═══════════════════════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);
    
    if (session.task_count > 0) {
        print_graph_chart();
        print_statistics();
    } else {
        printf("%s⚠ %sNo tasks recorded in this session.%s\n", 
               COLOR_YELLOW, COLOR_BOLD, COLOR_RESET);
    }
    
    printf("\n%s", COLOR_CYAN);
    print_divider();
    printf("%s", COLOR_RESET);
}

/* ============================================================================
 * Main Session Loop
 * ============================================================================ */

void run_session(void) {
    int should_continue = 1;
    
    while (should_continue) {
        char task_name[MAX_TASK_NAME];
        
        /* Get task name */
        read_task_name(task_name, sizeof(task_name));
        
        if (strlen(task_name) == 0) {
            printf("%s✗ %sTask name cannot be empty. Try again.%s\n", 
                   COLOR_RED, COLOR_BOLD, COLOR_RESET);
            continue;
        }
        
        /* Run timer */
        long elapsed = run_timer(task_name);
        
        /* Record task */
        record_task(task_name, elapsed);
        
        /* Show task result */
        printf("\n%s━ Task Completed ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", 
               COLOR_GREEN, COLOR_RESET);
        printf("%s✓%s %s%s%s\n", COLOR_GREEN, COLOR_RESET, COLOR_BOLD, task_name, COLOR_RESET);
        
        char time_str[16];
        format_time(elapsed, time_str, sizeof(time_str));
        printf("%s  Time spent: %s%s%s\n", 
               COLOR_DIM, COLOR_YELLOW, time_str, COLOR_RESET);
        
        /* Show menu */
        int choice = show_task_menu();
        if (choice == 2) {
            should_continue = 0;
        }
    }
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

int main(void) {
    clear_screen();
    print_banner();
    printf("\n%s  Press %s[Enter]%s to begin%s\n\n", 
           COLOR_DIM, COLOR_YELLOW, COLOR_DIM, COLOR_RESET);
    getchar();
    
    run_session();
    
    clear_screen();
    print_session_summary();
    printf("\n%s👋 Session ended. Thank you for focusing!%s\n\n", COLOR_CYAN, COLOR_RESET);
    
    return 0;
}
