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
- normal move and insert mode
- change mode with `i` and `a`
- add new line and change mode with `o` and `O`
- move with `h`, `j`, `k`, `l` in normal mode
- move to start of line with `$` and end of line with `^`
- move to start of file with `gg` and end with `G`
- commands `:w`, `:q`, `:wq`, `:q!`
- command `:saveas [NAME]` to save files
- search with `/[SEARCH]`, `n` for next occurrence and `N` for previous

##
[![forthebadge](https://forthebadge.com/images/badges/made-with-c.svg)](https://forthebadge.com)
[![forthebadge](https://forthebadge.com/images/badges/you-didnt-ask-for-this.svg)](https://forthebadge.com)
