These instructions are for Gemini. At this point use of Gemini
is experimental and focusing on basic tasks such as reviewing
documentation and adding translations.

# Git workflow
Stay on the current Git branch. Do NOT change the branch.

Create only local commits on that branch. Do NOT push commits
to a remote.

Do not delete/amend any existing commits.

Start commit messages with a verb in imperative, e.g. "Fix …".

# Project structure
There is a top-level CMake project which includes nested CMake
projects from sub directories. The CMake projects in sub
directories list all relevant files for their sub directory.

This list can be used to find source code files but also
additional files like translations. When adding/removing files,
the CMake project files need to be updated.

# Building
Do NOT invoke CMake. Do NOT try to build/compile anything. The
tasks I will give you for now will NOT require you to build
anything for now.

# Localization
Translations are done using the internationalization features
provided by the Qt framework.

The source tree contains "translations" sub folders with "ts"
files in them. There is one "ts" file for each locale. To
add/update translations, these "ts" files need to be updated.
When updating these files, take care to retain a valid XML
structure, e.g. use escape characters as needed. Escape also
quotes even if this is not necessary to be consistent with
Qt Linguist.

The source code contains human readable text in English. This
is the "source of truth". So when changing English text you
have to change it at the source in addition to changing it in
"ts" files.

Keep the `<translations>` tag empty if it doesn't differ from
the `<source>` tag. Only ensure the `type="unfinished"`
attribute is removed in this case.

Ensure punctuation and whitespaces are preserved, e.g. if the
source text ends with a whitespace (like "Some field: "),
preserve the trailing whitespace (like "Ein Feld: ").

Avoid using "Sie" in German translations. So write e.g."Öffne …"
and not "Öffnen Sie …".

# Misc
Keep wording/phrasing in-line with what is already there.
Feel free to fix typos or grammar mistakes, though. Do this
when updating translations but also when updating documentation,
comments and code.
