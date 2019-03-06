find ./src -regex '.*\.\(cpp\|hpp\)' -exec clang-format -style=file -i {} \;
