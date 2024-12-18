RM			=	rm -rf

SRCS_DIR	=	srcs/
INCS_DIR	=	srcs/
BUILD_DIR	=	build/

SRCS		=	$(wildcard $(SRCS_DIR)*.cpp)
HEADERS		=	$(wildcard $(INCS_DIR)*.hpp)

test:
	@ cmake -S . -B build
	@ cmake --build build
	@ cd build && ctest --output-on-failure

coverage: test
	@ cd build && lcov --capture --directory ./CMakeFiles --output-file coverage.info --rc "lcov_branch_coverage=1"
	@ cd build && lcov --remove coverage.info '/usr/*' '*/googletest/*' '*/test/*' --output-file coverage.filtered.info  --rc "lcov_branch_coverage=1"
	@ cd build && genhtml coverage.filtered.info --output-directory coverage_report --branch-coverage
	@ echo "\nCoverage Report:\n./test/build/coverage_report/index.html\n"

fclean:
	$(RM) $(BUILD_DIR)

fmt:
	$(if $(SRCS), clang-format --style=Google -i $(SRCS))
	$(if $(HEADERS), clang-format --style=Google -i $(HEADERS))

.PHONY:	test fclean fmt
