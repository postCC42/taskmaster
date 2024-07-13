NAME := taskmaster
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build

SRC_FILES := main.cpp Command.cpp ConfigManager.cpp Logger.cpp Process.cpp TaskMaster.cpp Utils.cpp
SRCS := $(addprefix $(SRC_DIR)/,$(SRC_FILES))
OBJS := $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(SRC_FILES:%.cpp=$(BUILD_DIR)/%.d)
CXX_DEFS := NAME=\"$(NAME)\"

CXX := g++
CXX_FLAGS := -Wextra -Werror -Wall -std=c++17 -g3
CXX_LINKS := -L$(LIB_DIR)
CXX_LIBS := 

CXX_HEADERS := -I$(INCLUDE_DIR)

CXX_DEPS_FLAGS := -MP -MMD
CXX_DEFS_FLAGS := $(foreach def,$(CXX_DEFS),-D $(def))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXX_FLAGS) $(CXX_HEADERS) $(OBJS) $(CXX_LIBS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXX_FLAGS) $(CXX_DEPS_FLAGS) $(CXX_DEFS_FLAGS) $(CXX_HEADERS) -c $< -o $@

-include $(DEPS)

clean:
	@rm -rf $(BUILD_DIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
