# minivim

## info

A "mini" implementation of vim :3

It uses `VT100` escape characters (I will implement `ncurses` in the future probably).

P.S. I know they are way too much comments, sorry for that. I do this project for learning purpouses, so I comment everything I do for that reason ":D

## How to  use

- Clone the repo

```sh
git clone https://github.com/izenynn/minivim.git
```

- Run make inside the repo

```sh
cd ./minivim && make
```

- Open a file using minivim

```sh
./minivim [FILE]
```

- Create a new file

```sh
./minivim [FILE]
```

- or name it later

```sh
./minivim
```

## Features

Editor features:
- Open, edit and save any text files.
- C and C++ syntax highlighting.

Vim features:
- `normal` and `insert` mode.
- `i`, `a`: change to insert mode.
- `o`, `O`: insert new line.
- `h`, `j`, `k`, `l`: move around (also: arrows).
- `0`: move to first character in the line (also: home key).
- `^`: move to first non-blank character in the line.
- `$`: move to last character in the line (also: end key).
- `gg`: goto first line.
- `G`: goto last line.
- `:w`, `:q`, `:q!`, `:wq`: supported commands.
- `:saveas [NAME]`: supported command.
- `/[MATCH]`: supported command (`n`, `N`: move to next and previous occurrence).

##
[![forthebadge](https://forthebadge.com/images/badges/made-with-c.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/you-didnt-ask-for-this.svg)](https://forthebadge.com)
