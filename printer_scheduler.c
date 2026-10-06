/*
 * Assignment 2 - Printer Scheduling System
 * Course : 21CSC201J Data Structures and Algorithms
 * Faculty: Dr. S. Shanmuga Priya
 * Team   : Rahasya Pandey (RA2511056050008)
 *          Shamil Hussain (RA2511056050010)
 *
 * Data structures:
 *   - Normal jobs : Queue (singly linked list, FIFO)
 *   - Urgent jobs : Priority Queue (binary min-heap, dynamic array)
 * Fairness rule: after FAIRNESS_LIMIT consecutive urgent jobs, one
 * waiting normal job is served (prevents starvation).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LEN 50
#define FAIRNESS_LIMIT 3
#define INIT_CAP 4

typedef struct {
    int id;
    char name[NAME_LEN];
    int pages;
    int priority;               /* 1 = highest ... 3 = lowest (urgent only) */
} Job;

typedef struct Node {
    Job job;
    struct Node *next;
} Node;

typedef struct {
    Node *front, *rear;
    int size;
} Queue;

typedef struct {
    Job *data;
    int size, capacity;
} PriorityQueue;

typedef struct {
    Queue normal;
    PriorityQueue urgent;
    int nextId;
    int urgentStreak;
} Scheduler;

/* ---------------- Queue (normal jobs) ---------------- */
void q_init(Queue *q) { q->front = q->rear = NULL; q->size = 0; }
int q_empty(const Queue *q) { return q->front == NULL; }

int q_enqueue(Queue *q, Job j) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) return 0;
    n->job = j;
    n->next = NULL;
    if (q->rear) q->rear->next = n; else q->front = n;
    q->rear = n;
    q->size++;
    return 1;
}

int q_dequeue(Queue *q, Job *out) {
    if (q_empty(q)) return 0;
    Node *n = q->front;
    *out = n->job;
    q->front = n->next;
    if (!q->front) q->rear = NULL;
    free(n);
    q->size--;
    return 1;
}

int q_remove(Queue *q, int id) {
    Node *prev = NULL, *cur = q->front;
    while (cur && cur->job.id != id) { prev = cur; cur = cur->next; }
    if (!cur) return 0;
    if (prev) prev->next = cur->next; else q->front = cur->next;
    if (cur == q->rear) q->rear = prev;
    free(cur);
    q->size--;
    return 1;
}

void q_free(Queue *q) { Job tmp; while (q_dequeue(q, &tmp)); }

#ifndef __wasm__
void q_display(const Queue *q) {
    const Node *n = q->front;
    if (!n) { printf("  (no normal jobs)\n"); return; }
    for (; n; n = n->next)
        printf("  ID %-3d | %-20s | %3d pages\n", n->job.id, n->job.name, n->job.pages);
}
#endif

/* ---------------- Priority Queue (urgent jobs) ---------------- */
static int higher(const Job *a, const Job *b) {      /* a should be served before b */
    if (a->priority != b->priority) return a->priority < b->priority;
    return a->id < b->id;                              /* tie -> earlier job first */
}

static void swap_jobs(Job *a, Job *b) { Job t = *a; *a = *b; *b = t; }

int pq_init(PriorityQueue *pq) {
    pq->data = (Job *)malloc(INIT_CAP * sizeof(Job));
    pq->size = 0;
    pq->capacity = pq->data ? INIT_CAP : 0;
    return pq->data != NULL;
}
int pq_empty(const PriorityQueue *pq) { return pq->size == 0; }

static void sift_up(PriorityQueue *pq, int i) {
    while (i > 0) {
        int p = (i - 1) / 2;
        if (!higher(&pq->data[i], &pq->data[p])) break;
        swap_jobs(&pq->data[i], &pq->data[p]);
        i = p;
    }
}

static void sift_down(PriorityQueue *pq, int i) {
    for (;;) {
        int l = 2 * i + 1, r = l + 1, best = i;
        if (l < pq->size && higher(&pq->data[l], &pq->data[best])) best = l;
        if (r < pq->size && higher(&pq->data[r], &pq->data[best])) best = r;
        if (best == i) break;
        swap_jobs(&pq->data[i], &pq->data[best]);
        i = best;
    }
}

int pq_push(PriorityQueue *pq, Job j) {
    if (pq->size == pq->capacity) {                    /* "full" -> grow dynamically */
        int newCap = pq->capacity ? pq->capacity * 2 : INIT_CAP;
        Job *tmp = (Job *)realloc(pq->data, newCap * sizeof(Job));
        if (!tmp) return 0;
        pq->data = tmp;
        pq->capacity = newCap;
    }
    pq->data[pq->size] = j;
    sift_up(pq, pq->size++);
    return 1;
}

int pq_pop(PriorityQueue *pq, Job *out) {
    if (pq_empty(pq)) return 0;
    *out = pq->data[0];
    pq->data[0] = pq->data[--pq->size];
    sift_down(pq, 0);
    return 1;
}

int pq_remove(PriorityQueue *pq, int id) {
    for (int i = 0; i < pq->size; i++) {
        if (pq->data[i].id == id) {
            pq->data[i] = pq->data[--pq->size];
            if (i < pq->size) { sift_down(pq, i); sift_up(pq, i); }
            return 1;
        }
    }
    return 0;
}

void pq_free(PriorityQueue *pq) { free(pq->data); pq->data = NULL; pq->size = pq->capacity = 0; }

#ifndef __wasm__
void pq_display(const PriorityQueue *pq) {
    if (pq_empty(pq)) { printf("  (no urgent jobs)\n"); return; }
    /* copy so the heap is not disturbed, then pop in order */
    PriorityQueue tmp;
    tmp.data = (Job *)malloc(pq->capacity * sizeof(Job));
    if (!tmp.data) return;
    memcpy(tmp.data, pq->data, pq->size * sizeof(Job));
    tmp.size = pq->size; tmp.capacity = pq->capacity;
    Job j;
    while (pq_pop(&tmp, &j))
        printf("  ID %-3d | %-20s | %3d pages | priority %d\n", j.id, j.name, j.pages, j.priority);
    free(tmp.data);
}
#endif

/* ---------------- Scheduler ---------------- */
int sched_init(Scheduler *s) {
    q_init(&s->normal);
    s->nextId = 1;
    s->urgentStreak = 0;
    return pq_init(&s->urgent);
}

void sched_free(Scheduler *s) { q_free(&s->normal); pq_free(&s->urgent); }

int sched_add(Scheduler *s, const char *name, int pages, int urgent, int priority) {
    Job j;
    j.id = s->nextId;
    strncpy(j.name, name, NAME_LEN - 1);
    j.name[NAME_LEN - 1] = '\0';
    j.pages = pages;
    j.priority = urgent ? priority : 0;
    int ok = urgent ? pq_push(&s->urgent, j) : q_enqueue(&s->normal, j);
    if (ok) s->nextId++;
    return ok ? j.id : -1;
}

int sched_cancel(Scheduler *s, int id) {
    return q_remove(&s->normal, id) || pq_remove(&s->urgent, id);
}

int sched_process(Scheduler *s, Job *out, int *wasUrgent) {
    if (q_empty(&s->normal) && pq_empty(&s->urgent)) return 0;
    int useUrgent = !pq_empty(&s->urgent) &&
                    (q_empty(&s->normal) || s->urgentStreak < FAIRNESS_LIMIT);
    if (useUrgent) { pq_pop(&s->urgent, out); s->urgentStreak++; }
    else           { q_dequeue(&s->normal, out); s->urgentStreak = 0; }
    *wasUrgent = useUrgent;
    return 1;
}

/* ======================================================================
 * WebAssembly API: the SAME scheduler core, exposed to the web front end.
 * Build:  zig cc -target wasm32-wasi -O2 -mexec-model=reactor -o scheduler.wasm printer_scheduler.c
 * ====================================================================== */
#ifdef __wasm__
#define EXPORT(n) __attribute__((export_name(n)))

static Scheduler g;
static char nameBuf[NAME_LEN];
static char *json = NULL;
static int doneCount = 0, lastValid = 0, lastUrgent = 0;
static Job lastJob;

EXPORT("name_buf") char *name_buf(void) { return nameBuf; }

EXPORT("api_init") int api_init(void) {
    sched_free(&g);
    free(json); json = NULL;
    doneCount = lastValid = 0;
    return sched_init(&g);
}

/* returns job id, or -1 empty name, -2 bad pages, -3 bad priority, -4 out of memory */
EXPORT("api_add") int api_add(int pages, int urgent, int priority) {
    nameBuf[NAME_LEN - 1] = '\0';
    if (nameBuf[0] == '\0') return -1;
    if (pages < 1 || pages > 500) return -2;
    if (urgent && (priority < 1 || priority > 3)) return -3;
    int id = sched_add(&g, nameBuf, pages, urgent, priority);
    return id < 0 ? -4 : id;
}

EXPORT("api_cancel") int api_cancel(int id) { return sched_cancel(&g, id); }

EXPORT("api_process") int api_process(void) {
    int u;
    if (!sched_process(&g, &lastJob, &u)) return 0;
    lastUrgent = u; lastValid = 1; doneCount++;
    return lastJob.id;
}

static int put_job(char *p, int cap, const Job *j, int urg) {
    char nm[NAME_LEN * 2]; int k = 0;
    for (const char *c = j->name; *c && k < (int)sizeof nm - 2; c++) {
        if (*c == '"' || *c == '\\') nm[k++] = '\'';
        else if ((unsigned char)*c >= 32) nm[k++] = *c;
    }
    nm[k] = '\0';
    return snprintf(p, cap, "{\"id\":%d,\"name\":\"%s\",\"pages\":%d,\"priority\":%d,\"urgent\":%d}", j->id, nm, j->pages, j->priority, urg);
}

/* Whole scheduler state as JSON (urgent list in true priority order). */
EXPORT("api_state") const char *api_state(void) {
    int cap = (g.normal.size + g.urgent.size + 2) * 160 + 256, n = 0;
    free(json);
    json = (char *)malloc(cap);
    if (!json) return "{}";
    n += snprintf(json + n, cap - n, "{\"done\":%d,\"streak\":%d,\"limit\":%d,\"nextUrgent\":%d,\"urgent\":[",
                  doneCount, g.urgentStreak, FAIRNESS_LIMIT,
                  !pq_empty(&g.urgent) && (q_empty(&g.normal) || g.urgentStreak < FAIRNESS_LIMIT));
    PriorityQueue tmp;                         /* copy heap, pop in order */
    tmp.size = g.urgent.size; tmp.capacity = g.urgent.capacity;
    tmp.data = (Job *)malloc((tmp.capacity ? tmp.capacity : 1) * sizeof(Job));
    if (tmp.data) {
        memcpy(tmp.data, g.urgent.data, tmp.size * sizeof(Job));
        Job j; int first = 1;
        while (pq_pop(&tmp, &j)) {
            if (!first) json[n++] = ',';
            first = 0; n += put_job(json + n, cap - n, &j, 1);
        }
        free(tmp.data);
    }
    n += snprintf(json + n, cap - n, "],\"normal\":[");
    for (Node *x = g.normal.front; x; x = x->next) {
        n += put_job(json + n, cap - n, &x->job, 0);
        if (x->next) json[n++] = ',';
    }
    n += snprintf(json + n, cap - n, "],\"last\":");
    if (lastValid) {
        n += put_job(json + n, cap - n, &lastJob, lastUrgent);
    } else n += snprintf(json + n, cap - n, "null");
    snprintf(json + n, cap - n, "}");
    return json;
}
#endif /* __wasm__ */

/* ======================================================================
 * Command-line (menu-driven) interface - native build only
 * ====================================================================== */
#ifndef __wasm__
/* ---------------- Input helpers ---------------- */
int read_int(const char *prompt, int lo, int hi, int *out) {
    char buf[64], *end;
    for (int tries = 0; tries < 5; tries++) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof buf, stdin)) return 0;
        long v = strtol(buf, &end, 10);
        if (end != buf && (*end == '\n' || *end == '\0') && v >= lo && v <= hi) {
            *out = (int)v;
            return 1;
        }
        printf("  Invalid input. Enter a number between %d and %d.\n", lo, hi);
    }
    return 0;
}

int read_name(char *dst, int size) {
    printf("Document name: ");
    if (!fgets(dst, size, stdin)) return 0;
    dst[strcspn(dst, "\n")] = '\0';
    return dst[0] != '\0';
}

void menu(void) {
    printf("\n===== PRINTER SCHEDULING SYSTEM =====\n"
           "1. Add normal print job\n"
           "2. Add urgent print job\n"
           "3. Cancel a job\n"
           "4. Process next job\n"
           "5. Display pending jobs\n"
           "6. Exit\n");
}

int main(void) {
    Scheduler s;
    if (!sched_init(&s)) { printf("Memory allocation failed.\n"); return 1; }

    int choice;
    for (;;) {
        menu();
        if (!read_int("Enter choice: ", 1, 6, &choice)) break;
        if (choice == 6) break;

        if (choice == 1 || choice == 2) {
            char name[NAME_LEN];
            int pages, pr = 0;
            if (!read_name(name, sizeof name)) { printf("Name cannot be empty.\n"); continue; }
            if (!read_int("Number of pages (1-500): ", 1, 500, &pages)) continue;
            if (choice == 2 && !read_int("Priority (1=highest, 3=lowest): ", 1, 3, &pr)) continue;
            int id = sched_add(&s, name, pages, choice == 2, pr);
            if (id < 0) printf("Could not add job (out of memory).\n");
            else printf("Job added with ID %d.\n", id);
        } else if (choice == 3) {
            int id;
            if (q_empty(&s.normal) && pq_empty(&s.urgent)) { printf("Nothing to cancel - queues are empty.\n"); continue; }
            if (!read_int("Job ID to cancel: ", 1, 1000000, &id)) continue;
            printf(sched_cancel(&s, id) ? "Job %d cancelled.\n" : "Job %d not found.\n", id);
        } else if (choice == 4) {
            Job j; int urgent;
            if (!sched_process(&s, &j, &urgent)) { printf("No jobs to process - queues are empty.\n"); continue; }
            printf("Printing %s job ID %d: %s (%d pages)\n", urgent ? "URGENT" : "NORMAL", j.id, j.name, j.pages);
        } else if (choice == 5) {
            printf("\nUrgent jobs (served by priority):\n");
            pq_display(&s.urgent);
            printf("Normal jobs (served FIFO):\n");
            q_display(&s.normal);
        }
    }

    sched_free(&s);
    printf("All memory freed. Goodbye!\n");
    return 0;
}
#endif /* !__wasm__ */
