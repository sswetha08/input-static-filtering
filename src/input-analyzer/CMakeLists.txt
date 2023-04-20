set(LLVM_LINK_COMPONENTS support)

add_clang_executable(input-analyzer
  InputAnalyzer.cpp
  )
target_link_libraries(input-analyzer
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  )