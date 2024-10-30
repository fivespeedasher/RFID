# 添加路径下所有源文件到DIR_LIB_SRCS变量
aux_source_directory(. DIR_LIB_SRCS)

# 指定生成IncludeSrc链接库
add_library(IncludeReader ${DIR_LIB_SRCS})