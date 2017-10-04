A container root process
========================

Start multiple tasks, and kill everything when one of them dies.
Also run commands on a regular basis.
Kill children when killed.
Clean up zombie processes.

Basic:
```
ubiq name=first-process /app/the-one args args \
 --- name=second-process /app/the-other \
 --- name=scheduled pause=360 /app/cleanup

```
Noteworthy: No configuration file, just give all the processes with arguments,
separated with `---`. At the begin of each command line there can be options
for that job. `name=` sets a name for logging information (otherwise the program
name is used), `pause=S` makes a job regularly run, with a pause of S seconds
in between. `dir=D` executes the program in D as the current directory.
`su=U` executes the program with the user and group id of the user U, and also
sets `$HOME`, `$USER`, and `$LOGNAME` accordingly.

The program name must be a full path, no `$PATH` search is performed.

When a process without `pause`, i.e. not a regularly run one, died, `ubik`
terminates the other processes and exits when all have terminated.

Finding a backronym is left as an exercise to the reader.
