# BearOS shell

The BearOS shell (command-line interpreter) looks and behaves a little like
a Linux shell, but not similarly enough to be all that helpful. Owing
to the fact that the shell has to fit into the Pico flash, only limited
funtionality is provided. 

However, the shell has a number of built-in commands, and can handle
input and output redirection (using the same syntax as Linux). It can
find and launch programs from the SD card.

## Built in shell commands

cat - displays one or more files to stdout
cd - change working directory
clear - clears the screen
cp - copies one or more files
date - get or set the date and time
env - print or set environment variables
df - display free disk space
echo - print the arguments to stdout
gpio - read or set GPIO pins
grep - search for patterns in files 
ls - list directory contents
mkdir - create directories
mv - moves one or more files
rm - removes one or more files
rmdir - removes one or more (empty) directories
source - run a script
uname - print sytem information

These commands operate somewhat like the GNU/Linux utilities with the same
names, but are generally a lot less sophisticated.

## External programs

The shell uses the environment variable `PATH`, which defaults to `A:/bin`,
to look for external programs. BearOS will only successfully run programs
specifically compiled for it, although it might try and fail to run other
ARM binaries of the same format (ELF).

## Redirection

Standard input and standard output can be redirected, using the '<' and '>'
symbols. The meanings are the same as in Linux:

- prog > file -- store output from prog in file
- prog >> file -- append output from prog to file
- prog < file -- pass file as input to prog

stderr cannot at present be redirected, and always goes to the console.

The BearOS shell does not support multiple redirections of the same type.
So in 'myprog > a >> b` the redirection to `b` is ignored, and a warning
will be printed to `stderr`. The same applies when trying to put a
command in a pipeline when its input or output have been redirected in
such a way as to render the pipe useless.

## Environment

A statement of the form `foo=bar` sets the environment variable `foo` to
`bar`. `foo=` clears the variable. Variables can be used in shell commands
and scripts using the `${foo}` syntax. Note that if a command is run
like this:

    foo=bar myprog

the the variable is set only for the environment of `myprog`. This is the
same as on Linux but, in general, Linux environment behaviour cannot be
relied on in BearOS. For example

   foo=bar echo ${foo}

does not produce the same output as on Linux, because the BearOS shell's
parsing process is completely different from that of Linux shells. 

The `env` command with no arguments displays the current environment. 
`env` can also set environment variables, but it's just as easy to use
the `boo=bar` syntax.

## Quoting and escaping

The double-quote character removes the special status of most special
characters. So, for example, "\#" prevents the `#` being treated as
introducing a comment. `\\` reduces to a single `\`.

The double-quote character has the same effect, but over a number of 
characters. Double-quote takes preference over `\`, so `"\foo"` reduces
to `\foo`. However, to use the double-quote in an argument, represent it
as `\"`. 

## Shell scripts

There is very limited shell scripting support. The command `source {filename}`
reads a file one line at a time and processes it, as if the lines had
been entered on the command line. Any changes made to the environment
in the script are retained in the shell. 

Arguments can be passed to shell scripts on the command line. Inside the
script, arguments can be retrieved as `${1}`, `${2}`, etc. As is conventional,
`${0}` is the name of the script file.

## Comments

Comments start with a `#` character, and extend to the end of the line.
To use the hash character, enclose it in double-quotes.

## Notes

All the shell built-in commands are crude and poorly-featured compared to their
Linux namesakes. 

Most shell commands take a '-h' switch to show the options available, but
this is not universally implemented at present. 

The `df` command can be _very_ slow on large SD cards (minutes). But it
isn't always slow, and I'm not sure I understand why.

External commands are searched before shell internal commands, so that the
shell versions can be replaced by better, external alternatives.

Files can be shared with, for example, Linux or Windows systems using the SD
card. However, be aware that all the built-in commands that handle text files
expect the end-of-line indicator to be a single line feed.  This is the
convention on Linux/Unix, but Windows uses carriage return/ line feed.

Pipelines are implemented using temporary files (because this is a
single-tasking system). There's no speed advantage to using pipelines over
discrete commands, and the second command won't start until the first has
completed. For example, 'ls | grep txt' is exactly the same as 'ls >
/tmp/foo; grep txt < /tmp/foo'. This is how the MSDOS `COMMAND.COM` works :) Of
course, it is often convenient to use the pipeline syntax. It goes without
saying that there needs to be space on the SD card to store the temporary
files, and that the directory indicated by the `TMP` environment variable
exists.

Double-quoted strings are automatically terminated.  Unterminated redirections
are quietyly ignored, as are pipes that don't connect to anything. Unlike a
Linux shell, the BearOS shell won't prompt for further input to make good
errors like this. 

## Notes on specific commands

`grep` uses 'picoregex' rather than a full POSIX-compliant regular expression
matcher, for reasons of size. It won't offer the full pattern set of a Unix
`grep` utility.

`cat` is _buffered_ by default, for reasons of speed. That is, it reads and
writes in blocks. To read and write character-by-character, use the `-u`
switch.
 
