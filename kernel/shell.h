#ifndef SHELL_H
#define SHELL_H

/* Initialise the shell (reserved for future setup; currently a no-op) */
void shell_init(void);

/* Enter the interactive command loop – never returns */
void shell_run(void);

#endif /* SHELL_H */
