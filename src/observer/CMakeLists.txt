PROJECT(observer)
MESSAGE("Begin to build " ${PROJECT_NAME})
MESSAGE(STATUS "This is PROJECT_BINARY_DIR dir " ${PROJECT_BINARY_DIR})
MESSAGE(STATUS "This is PROJECT_SOURCE_DIR dir " ${PROJECT_SOURCE_DIR})


#INCLUDE_DIRECTORIES([AFTER|BEFORE] [SYSTEM] dir1 dir2 ...)
INCLUDE_DIRECTORIES(. ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/../../deps /usr/local/include SYSTEM)
# 父cmake 设置的include_directories 和link_directories并不传导到子cmake里面
#INCLUDE_DIRECTORIES(BEFORE ${CMAKE_INSTALL_PREFIX}/include)
LINK_DIRECTORIES(/usr/local/lib ${PROJECT_BINARY_DIR}/../../lib)


FILE(GLOB_RECURSE ALL_SRC *.cpp *.c)
FILE(GLOB MAIN_SRC main.cpp)
MESSAGE("MAIN SRC: " ${MAIN_SRC})
FOREACH (F ${ALL_SRC})

    IF (NOT ${F} STREQUAL ${MAIN_SRC})
        SET(LIB_SRC ${LIB_SRC} ${F} sql/expr/TupleSet.cpp sql/expr/TupleSet.h util/descartes.h util/descartes.cpp sql/operator/pred_mutitable_operator.h sql/operator/pred_mutitable_operator.cpp)
    ENDIF()

    MESSAGE("Use " ${F})

ENDFOREACH (F)

SET(LIBRARIES common pthread dl event jsoncpp)

# 指定目标文件位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../../bin)
MESSAGE("Binary directory:" ${EXECUTABLE_OUTPUT_PATH})
ADD_EXECUTABLE(${PROJECT_NAME} ${ALL_SRC})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBRARIES})

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../../lib)
MESSAGE("Archive directory:" ${LIBRARY_OUTPUT_PATH})
ADD_LIBRARY(${PROJECT_NAME}_static STATIC ${LIB_SRC})
SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
TARGET_LINK_LIBRARIES(${PROJECT_NAME}_static ${LIBRARIES})


# Target 必须在定义 ADD_EXECUTABLE 之后， programs 不受这个限制
# TARGETS和PROGRAMS 的默认权限是OWNER_EXECUTE, GROUP_EXECUTE, 和WORLD_EXECUTE，即755权限， programs 都是处理脚本类
# 类型分为RUNTIME／LIBRARY／ARCHIVE, prog
INSTALL(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static 
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib)
