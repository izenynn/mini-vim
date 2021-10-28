######################################################################
#                                VARS                                #
######################################################################

TAB_SIZE = 4

######################################################################
#                                NAME                                #
######################################################################

NAME = minivim
#NAME = mvm

######################################################################
#                              COMPILER                              #
######################################################################

CC = gcc

# only dev flags
CFLAGS += -Wall -Werror -Wextra -pedantic-errors
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=bounds -fsanitize=null -g3

# flags
CFLAGS += -D TAB_SIZE=$(TAB_SIZE)

#LDFLAGS = -L

# ncurses will be probably implemented in the future, right now only supports VT100 escapes
#LDLIBS = -lncurses

######################################################################
#                                 RM                                 #
######################################################################

RM = rm

RMFLAGS = -rf

######################################################################
#                                SRC                                 #
######################################################################

SRC = minivim.c

OBJ = $(SRC:%.c=%.o)

######################################################################
#                               RULES                                #
######################################################################

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(RMFLAGS) $(OBJ)

fclean: clean
	$(RM) $(RMFLAGS) $(NAME)

re: fclean all
