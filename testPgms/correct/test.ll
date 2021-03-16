; ModuleID = 'TEST'
source_filename = "TEST"

@I = common global i32 0
@TMP = common global i1 false

declare i1 @PUTINTEGER(i32)

define i32 @main() {
entrypoint:
  store i32 0, i32* @I, align 4
  br label %0

0:                                                ; preds = %3, %entrypoint
  %1 = load i32, i32* @I, align 4
  %2 = icmp slt i32 %1, 5
  br i1 %2, label %3, label %8

3:                                                ; preds = %0
  %4 = load i32, i32* @I, align 4
  %5 = add i32 %4, 1
  store i32 %5, i32* @I, align 4
  %6 = load i32, i32* @I, align 4
  %7 = call i1 @PUTINTEGER(i32 %6)
  store i1 %7, i1* @TMP, align 1
  br label %0

8:                                                ; preds = %0
  ret i32 0
}
