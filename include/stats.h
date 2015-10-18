#ifndef STATS_H
#define STATS_H

/* Structure used by 'get_stats' function */
struct stats
{
  unsigned long user_ticks;     /* Total ticks executed by the proocess */
  unsigned long system_ticks;   /* Total ticks executin systme code */
  unsigned long blocked_ticks;  /* Total ticks in the blocked state */
  unsigned long ready_ticks;    /* Total ticks in the ready state */
  unsigned long elapsed_total_ticks;  /* Ticks since the power on of the machine
                                    until the beginning of the current state */
  unsigned long total_trans; /* Number of times the process has got the CPU: READY->RUN transitions */
  unsigned long remaining_ticks;  /* Remaining ticks to en the quantum */
};
#endif /* !STATS_H */
