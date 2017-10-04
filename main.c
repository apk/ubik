#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>

#include <jfdt/exec.h>
#include <jfdt/opts.h>

static int debug = 0;

void tag (const char *fmt, ...) {
  static char buf [30];
  jfdtTime_t t = jfdtGetTime ();
  va_list ap;
  va_start (ap, fmt);
  printf ("%02d:%02d:%02d.%03d: ubik: ",
	   ((int)t.tv_sec / 3600) % 24,
	   ((int)t.tv_sec / 60) % 60,
	   (int)t.tv_sec % 60,
	   (int)t.tv_usec/1000);
  vprintf (fmt, ap);
  printf ("\n");
  va_end (ap);
  fflush (stdout);
}

char *signame (int sig) {
  static char buf [20];
  switch (sig) {
  case SIGHUP: return "hup";
  case SIGINT: return "int";
  case SIGQUIT: return "quit";
  case SIGILL: return "ill";
  case SIGABRT: return "abrt";
  case SIGFPE: return "fpe";
  case SIGKILL: return "kill";
  case SIGSEGV: return "segv";
  case SIGPIPE: return "pipe";
  case SIGALRM: return "alrm";
  case SIGTERM: return "term";
  case SIGUSR1: return "usr1";
  case SIGUSR2: return "usr2";
  case SIGCHLD: return "chld";
  case SIGCONT: return "cont";
  case SIGSTOP: return "stop";
  case SIGTSTP: return "tstp";
  case SIGTTIN: return "ttin";
  default: sprintf (buf, "%d", sig); return buf;
  }
}

static volatile int lastsig = 0;

static void sig_hdl (int sig) {
  lastsig = sig;
  jfdtExecTriggerAsync ();
}

static struct sigaction sig_act = { sig_hdl, 0, 0, 0, 0 };

static void setup_sig (int sig) {
  sigaction (sig, &sig_act, 0);
}

void stray (int pid, int status) {
  if (WIFSIGNALED (status)) {
    tag ("reaped pid %d died on %s", pid, signame (WTERMSIG (status)));
  } else if (WIFEXITED (status)) {
    int rc = WEXITSTATUS (status);
    if (rc) {
      tag ("reaped pid %d rc %d", pid, rc);
    } else {
      tag ("reaped pid %d", pid);
    }
  } else {
    /* Probably unreachable, the way we waitpid() */
    tag ("reaped pid %d with status %x", pid, status);
  }
}

struct job {
  struct job *next;
  char **param;
  char *name;
  char *dir;
  long interval;
  jfdtExec_t exe;
  jfdtTimer_t tim;
  int pid;
} *joblist = 0;

int mode = 0;

jfdtTimer_t down;

void killall (int sig) {
  struct job *j;
  int f;
  jfdtTime_t t = jfdtGetTime ();
  if (sig) tag ("shutdown (%s)", signame (sig));
  for (f = 1, j = joblist; j; j = j->next) {
    if (j->pid) f = 0;
  }
  if (f) {
    tag ("all down, exiting");
    exit (0);
  }
  if (sig == 0) return; /* Only checking */
  if (mode == SIGKILL) exit (1);
  if (!mode) {
    /* Do the unset only once */
    for (j = joblist; j; j = j->next) {
      if (j->pid) continue;
      if (j->interval) {
	jfdtTimerUnset (&j->tim);
      }
    }
  }
  mode = sig;
  for (j = joblist; j; j = j->next) {
    if (j->pid) {
      tag ("kill %s[%d] with %s", j->name, j->pid, signame (sig));
      kill (j->pid, sig);
    }
  }
  jfdtTimeAddSecs (&t, mode == SIGKILL ? 1 : 15);
  jfdtTimerSet (&down, t);
}

void downfire (jfdtTimer_t *tm, jfdtTime_t now) {
  killall (SIGKILL);
}

void set_timer (struct job *j) {
  jfdtTime_t t = jfdtGetTime ();
  jfdtTimeAddSecs (&t, j->interval);
  jfdtTimerSet (&j->tim, t);
  if (debug) tag ("start %s in %d", j->name, (int)j->interval);
}

static void term (jfdtExec_t *exe, int status) {
  struct job *j = exe->userdata;
  if (WIFSIGNALED (status)) {
    tag ("died %s on %s", j->name, signame (WTERMSIG (status)));
  } else if (WIFEXITED (status)) {
    int rc = WEXITSTATUS (status);
    if (debug || rc || j->interval < 1) tag ("exited %s rc %d", j->name, rc);
  } else {
    tag ("waited %s with status %x", j->name, status);
  }
  j->pid = 0;
  if (mode == 0) {
    if (j->interval > 0) {
      set_timer (j);
    } else {
      killall (SIGTERM);
    }
  } else {
    killall (0);
  }
}

void inter (jfdtExec_t *exe, void *xud) {
  struct job *j = exe->userdata;
  if (j->dir) {
    int r = chdir (j->dir);
    if (r == -1) {
      fprintf (stderr, "%s: failed to chdir to %s\n", j->name, j->dir);
    }
  }
}

void exec_job (struct job *j) {
  j->pid = jfdtExecDo (&j->exe, term, inter, 0, j->param, 0, j, 0);
  if (j->pid == -1) {
    jfdt_trace ("oops");
    exit (1);
  }
  if (debug) tag ("started %s as pid: %d", j->name, j->pid);
}

void fire (jfdtTimer_t *tm, jfdtTime_t now) {
  struct job *j = tm->userdata;
  exec_job (j);
}

void async () {
  static int thissig = 0;
  if (lastsig && !thissig) {
    thissig = lastsig;
    killall (thissig);
  }
}

struct job *mkjob (char **out) {
  struct job *j = malloc (sizeof (struct job));
  j->param = out;
  j->interval = 0;
  j->next = joblist;
  j->pid = 0;
  j->name = 0;
  j->dir = 0;
  joblist = j;
  jfdtTimerInit (&j->tim, fire, j);
  return j;
}

int main (int argc, char **argv) {
  int i;
  
  char **out = malloc (argc * sizeof (char *));
  struct job *h;
  struct job *j = mkjob (out);
  int in_opts = 1;
  for (i = 1; i < argc; i ++) {
    char *q;
    char *p = argv [i];
    if (in_opts) {
      if ((q = jfdtOptsIsPrefix (p, "name="))) {
	j->name = q;
	continue;
      }
      if ((q = jfdtOptsIsPrefix (p, "dir="))) {
	j->dir = q;
	continue;
      }
      if ((q = jfdtOptsIsPrefix (p, "pause="))) {
	int v;
	char *s = jfdtOptsParseNat (q, &v);
	if (!s) goto bailarg;
	j->interval = v > 0 ? v : 1;
	continue;
      }
    }
    in_opts = 0;
    if (strncmp (p, "---", 3)) {
      /* Collect what isn't ours */
      *out ++ = argv [i];
      continue;
    }
    p += 3;
    if (p [0] == '-') {
      /* Convert leading ---- to --- and collect as well */
      *out ++ = argv [i] + 1;
      continue;
    }
    *out ++ = 0; /* Terminate previous */
    j = mkjob (out);
    in_opts = 1;

    if (*p) {
      j->interval = strtol (p, &q, 10);
      if (*q) {
      bailarg:
	fprintf (stderr, "ubik: bad opt %s\n", argv [i]);
	exit (1);
      }
    }
  }
  *out = 0;

  /* Invert joblist to its proper order */
  h = 0;
  while ((j = joblist)) {
    joblist = j->next;
    j->next = h;
    h = j;
    if (!j->param [0]) {
      fprintf (stderr, "ubik: empty job\n", argv [i]);
      exit (1);
    }
    if (!j->name) {
      j->name = j->param [0];
    }
  }
  joblist = h;

  /* Show them */
  for (j = joblist; j; j = j->next) {
    printf ("[%s:%d]", j->name, (int)j->interval);
    for (i = 0; j->param [i]; i ++) {
      printf (" %s", j->param [i]);
    }
    printf ("\n");
    fflush (stdout);
  }

  jfdtTimerInit (&down, downfire, 0);

  jfdtExecSetStrayHandler (stray);

  jfdtExecAddAsyncHandler (async);

  setup_sig (SIGTERM);
  setup_sig (SIGINT);

  /* Start them */
  for (j = joblist; j; j = j->next) {
    if (j->interval > 0) {
      set_timer (j);
    } else {
      exec_job (j);
    }
  }

  jfdtServe ();
}
