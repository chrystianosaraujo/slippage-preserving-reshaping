if(TARGET polyscope::polyscope)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
    polyscope
    GIT_REPOSITORY https://github.com/chrystianosaraujo/polyscope
    )
FetchContent_MakeAvailable(polyscope)

add_library(polyscope::polyscope ALIAS polyscope)
set_target_properties(polyscope PROPERTIES FOLDER third_party)