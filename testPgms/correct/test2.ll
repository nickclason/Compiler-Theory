; ModuleID = 'TEST_PROGRAM'
source_filename = "TEST_PROGRAM"

@FIBB_RESULT = common global i32 0
@OUT = common global i1 false

declare i1 @PUTINTEGER(i32)

define i32 @proc_0(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  %5 = load i32, i32* %3, align 4
  %6 = icmp slt i32 %5, 0
  br i1 %6, label %7, label %9

7:                                                ; preds = %entrypoint
  store i32 -1, i32* %4, align 4
  %8 = load i32, i32* %4, align 4
  ret i32 %8

9:                                                ; preds = %entrypoint
  %10 = load i32, i32* %3, align 4
  %11 = icmp eq i32 %10, 0
  br i1 %11, label %12, label %14

12:                                               ; preds = %9
  store i32 0, i32* %4, align 4
  %13 = load i32, i32* %4, align 4
  ret i32 %13

14:                                               ; preds = %9
  %15 = load i32, i32* %3, align 4
  %16 = icmp eq i32 %15, 1
  br i1 %16, label %17, label %19

17:                                               ; preds = %14
  store i32 1, i32* %4, align 4
  %18 = load i32, i32* %4, align 4
  ret i32 %18

19:                                               ; preds = %14
  %20 = load i32, i32* %3, align 4
  %21 = sub i32 %20, 1
  %22 = call i32 @proc_0(i32 %21)
  store i32 %22, i32* %1, align 4
  %23 = load i32, i32* %3, align 4
  %24 = sub i32 %23, 2
  %25 = call i32 @proc_0(i32 %24)
  store i32 %25, i32* %2, align 4
  %26 = load i32, i32* %1, align 4
  %27 = load i32, i32* %2, align 4
  %28 = add i32 %26, %27
  store i32 %28, i32* %4, align 4
  %29 = load i32, i32* %4, align 4
  ret i32 %29
}

define i32 @main() {
entrypoint:
  store i32 -1234, i32* @FIBB_RESULT, align 4
  %0 = call i32 @proc_0(i32 12)
  store i32 %0, i32* @FIBB_RESULT, align 4
  %1 = load i32, i32* @FIBB_RESULT, align 4
  %2 = call i1 @PUTINTEGER(i32 %1)
  store i1 %2, i1* @OUT, align 1
  ret i32 0
}
