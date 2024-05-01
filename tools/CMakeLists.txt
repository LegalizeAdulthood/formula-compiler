add_executable(formula-compiler main.cpp)
target_link_libraries(formula-compiler PUBLIC formula)
target_folder(formula-compiler "Tools")
