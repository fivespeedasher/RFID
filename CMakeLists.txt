cmake_minimum_required(VERSION 3.22)

# 项目信息
project(RFID)

# 添加路径下所有源文件到DIR_SRCS变量
aux_source_directory(. DIR_SRCS)

# 添加子目录
add_subdirectory(Reader)

# 指定生成目标
add_executable(start_read ${DIR_SRCS})

# 加入多个链接库
target_link_libraries(start_read IncludeReader sqlite3)

