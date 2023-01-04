###################################
# ft_ping
###################################

NAME = ft_ping

CC = gcc

CFLAGS = -Wall -Wextra -Werror

INCLUDES = -I .

LIBRARIES = -lm

FILES = ft_ping time utils

SRC = $(addsuffix .c, $(FILES)) \

OBJ = $(SRC:c=o)


###################################
# Methods
###################################

all: $(NAME)

$(NAME): $(OBJ)
	@printf "\033[0;32m\nCompiling %s...\n" $(NAME)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(NAME) $(OBJ) $(LIBRARIES)
	@printf "\n\033[0mDone !\n"

%.o: %.c
	@printf "\033[0;33m"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@printf "\033[0;31m\nDeleting %s objects...\n" $(NAME)
	rm -f $(OBJ)
	@printf "\033[0m\n"

fclean:
	@printf "\033[0;31m\nDeleting %s objects...\n" $(NAME)
	rm -f $(OBJ)
	@printf "\nDeleting %s executable...\n" $(NAME)
	rm -f $(NAME)
	@printf "\033[0m\n"

re: fclean all


.PHONY: clean fclean re