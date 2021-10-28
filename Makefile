# NAME

NAME = minivim
#NAME = mvm

# COMPILER

CC = gcc

CFLAGS += -Wall -Werror -Wextra -pedantic-errors
CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=bounds -fsanitize=null -g3

# RM

RM = rm

RMFLAGS = -rf

# SRC

SRC = minivim.c

OBJ = $(SRC:%.c=%.o)

# RULES

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(RMFLAGS) $(OBJ)

fclean: clean
	$(RM) $(RMFLAGS) $(NAME)

re: fclean all
