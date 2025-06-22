find_package(asmjit CONFIG REQUIRED)
find_package(boost_parser CONFIG REQUIRED)

add_library(formula
    include/formula/Complex.h
    include/formula/formula.h
    formula.cpp
    functions.h functions.cpp
)
target_compile_options(formula PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_include_directories(formula PUBLIC include)
target_link_libraries(formula PRIVATE asmjit::asmjit Boost::parser)
target_folder(formula "Libraries")
