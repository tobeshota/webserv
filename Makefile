# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: yoshimurahiro <yoshimurahiro@student.42    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/09/12 09:34:52 by tobeshota         #+#    #+#              #
#    Updated: 2025/03/21 17:51:10 by yoshimurahi      ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME			=	webserv
CONTAINER		=	webserv
CC				=	c++
FLAGS			=	-Wall -Wextra -Werror -std=c++98 -pedantic-errors
RM				=	rm -rf

SRCS_DIR		=	srcs/
INCS_DIR		=	srcs/
OBJS_DIR		=	objs/

SRCS 			=   $(shell find $(SRCS_DIR) -type f -name "*.cpp")
HEADERS			=	$(wildcard $(INCS_DIR)*.hpp)
SHS				=	$(wildcard *.sh)
OBJS			=	$(patsubst %.cpp, $(OBJS_DIR)%.o, $(SRCS))

ifeq ($(MAKECMDGOALS), debug)
	FLAGS += -g -DDEBUG
endif

ifeq ($(MAKECMDGOALS), address)
	FLAGS += -g3 -fsanitize=address
endif

ifeq ($(MAKECMDGOALS), leaks)
	FLAGS += -g3 -fsanitize=address -fsanitize=leak
endif

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(FLAGS) -I $(INCS_DIR) $(OBJS) -o $@

$(OBJS_DIR)%.o : %.cpp
	mkdir -p $(@D)
	$(CC) $(FLAGS) -I $(INCS_DIR) -c $< -o $@

clean:
	$(RM) $(OBJS_DIR)

fclean: clean
	make fclean -C test/
	make fclean -C ./docs -f doc.mk
	$(RM) $(NAME)

re:		fclean all

up:
	docker compose up -d

run: up
	docker exec -it $(CONTAINER) /bin/bash

down:
	docker compose down

fmt:
	make fmt -C test/
	$(if $(SRCS), clang-format --style=Google -i $(SRCS))
	$(if $(HEADERS), clang-format --style=Google -i $(HEADERS))
	$(if $(SHS), shfmt -w $(SHS))

debug: re

address: re

leaks: re
	@echo "\033[1;33mRunning with LeakSanitizer...\033[0m"
	@echo "\033[0;34mNote: Export ASAN_OPTIONS=detect_leaks=1 before running the program\033[0m"
	@echo "Example: ASAN_OPTIONS=detect_leaks=1 ./$(NAME)"

test:
	@ make test -C test/

coverage:
	@ make coverage -C test/

doc:
	make doc -C docs/ -f doc.mk

.PHONY:	all clean fclean re up run down fmt debug address test coverage doc leaks
