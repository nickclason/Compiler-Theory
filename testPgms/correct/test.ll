; ModuleID = 'TEST'
source_filename = "TEST"

@I = common global i32 0
@J = common global i32 0
@TMP = common global i1 false

declare i1 @PUTINTEGER(i32)

define i32 @proc_0() {
entrypoint:
  store i32 1, i32* @I, align 4
  ret i32 0
}

define i32 @main() {
entrypoint:
  store i32 0, i32* @J, align 4
  br label %0

0:                                                ; preds = %3, %entrypoint
  %1 = load i32, i32* @J, align 4
  %2 = icmp slt i32 %1, 5
  br i1 %2, label %3, label %7

3:                                                ; preds = %0
  %4 = call i32 @proc_0()
  store i32 %4, i32* @I, align 4
  %5 = load i32, i32* @J, align 4
  %6 = add i32 %5, 1
  store i32 %6, i32* @J, align 4
  br label %0

7:                                                ; preds = %0
  ret i32 0
}
