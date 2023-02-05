## Shell bugs

Some freaky redirections can break the shell. For example 'prog < < file'.
I don't know at present whether the problem is in the redirection logic,
or just in the parser.

Multiple pipes break the shell. So, for example, `ls | grep foo` is OK, but
`ls | grep foo | grep bar` crashes.

I'm sure there are many, many other bugs.
