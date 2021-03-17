; ModuleID = 'MULTIPLEPROCS'
source_filename = "MULTIPLEPROCS"

@Y = common global i32 0
@TMP = common global i1 false

declare i1 @PUTINTEGER(i32)

define i32 @proc_0(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  store i32 %0, i32* %1, align 4
  %2 = load i32, i32* %1, align 4
  %3 = add i32 %2, 1
  store i32 %3, i32* %1, align 4
  %4 = load i32, i32* %1, align 4
  %5 = call i32 @proc_1(i32 %4)
  store i32 %5, i32* %1, align 4
  %6 = load i32, i32* %1, align 4
  ret i32 %6
}

define i32 @proc_1(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  store i32 %0, i32* %1, align 4
  %2 = load i32, i32* %1, align 4
  %3 = add i32 %2, 1
  store i32 %3, i32* %1, align 4
  %4 = load i32, i32* %1, align 4
  %5 = call i32 @proc_2(i32 %4)
  store i32 %5, i32* %1, align 4
  %6 = load i32, i32* %1, align 4
  ret i32 %6
}

define i32 @proc_2(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  store i32 %0, i32* %1, align 4
  %2 = load i32, i32* %1, align 4
  %3 = add i32 %2, 1
  store i32 %3, i32* %1, align 4
  %4 = load i32, i32* %1, align 4
  ret i32 %4
}

define i32 @main() {
entrypoint:
  %0 = load i32, i32* @Y, align 4
  %1 = call i32 @proc_0(i32 %0)
  store i32 %1, i32* @Y, align 4
  %2 = load i32, i32* @Y, align 4
  %3 = call i1 @PUTINTEGER(i32 %2)
  store i1 %3, i1* @TMP, align 1
  ret i32 0
}
