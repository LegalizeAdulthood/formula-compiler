find_package(asmjit CONFIG REQUIRED)
find_package(boost_parser CONFIG REQUIRED)

add_library(formula
    include/formula/formula.h
    formula.cpp
)
target_include_directories(formula PUBLIC include)
target_link_libraries(formula PRIVATE asmjit::asmjit Boost::parser)
target_folder(formula "Libraries")
