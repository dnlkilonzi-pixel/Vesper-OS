#ifndef SHELL_H
#define SHELL_H

/* Initialise the shell (sets up keyboard driver etc.) */
void shell_init(void);

/* Enter the interactive command loop – never returns */
void shell_run(void);

#endif /* SHELL_H */
