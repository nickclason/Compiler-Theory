; ModuleID = 'TEST_PROGRAM'
source_filename = "TEST_PROGRAM"

@A = common global i32 0
@TMP = common global i1 false

declare i1 @PUTINTEGER(i32)

define i32 @proc_0(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  store i32 3, i32* %1, align 4
  %4 = load i32, i32* %1, align 4
  store i32 %4, i32* %3, align 4
  %5 = load i32, i32* %2, align 4
  %6 = load i32, i32* %3, align 4
  %7 = mul i32 %6, 4
  %8 = add i32 %5, %7
  ret i32 %8
}

define i32 @main() {
entrypoint:
  store i32 3, i32* @A, align 4
  %0 = call i32 @proc_0(i32 3)
  store i32 %0, i32* @A, align 4
  %1 = load i32, i32* @A, align 4
  %2 = call i1 @PUTINTEGER(i32 %1)
  store i1 %2, i1* @TMP, align 1
  ret i32 0
}
