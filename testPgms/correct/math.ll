; ModuleID = 'MATHS'
source_filename = "MATHS"

@VALUE = common global i32 0
@TMP2 = common global float 0.000000e+00
@OUT = common global i1 false

declare i1 @PUTINTEGER(i32)

define i32 @proc_0(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, i32* %4, align 4
  %5 = load i32, i32* %4, align 4
  %6 = icmp eq i32 %5, 0
  br i1 %6, label %7, label %9

7:                                                ; preds = %entrypoint
  store i32 0, i32* %1, align 4
  %8 = load i32, i32* %1, align 4
  ret i32 %8

9:                                                ; preds = %entrypoint
  %10 = load i32, i32* %4, align 4
  %11 = icmp eq i32 %10, 1
  br i1 %11, label %12, label %14

12:                                               ; preds = %9
  store i32 1, i32* %1, align 4
  %13 = load i32, i32* %1, align 4
  ret i32 %13

14:                                               ; preds = %9
  %15 = load i32, i32* %4, align 4
  %16 = sub i32 %15, 1
  store i32 %16, i32* %4, align 4
  %17 = load i32, i32* %4, align 4
  %18 = call i32 @proc_0(i32 %17)
  store i32 %18, i32* %2, align 4
  %19 = load i32, i32* %4, align 4
  %20 = sub i32 %19, 1
  store i32 %20, i32* %4, align 4
  %21 = load i32, i32* %4, align 4
  %22 = call i32 @proc_0(i32 %21)
  store i32 %22, i32* %3, align 4
  %23 = load i32, i32* %2, align 4
  %24 = load i32, i32* %3, align 4
  %25 = add i32 %23, %24
  store i32 %25, i32* %1, align 4
  %26 = load i32, i32* %1, align 4
  ret i32 %26
}

define i32 @main() {
entrypoint:
  %0 = call i32 @proc_0(i32 15)
  store i32 %0, i32* @VALUE, align 4
  %1 = load i32, i32* @VALUE, align 4
  %2 = call i1 @PUTINTEGER(i32 %1)
  store i1 %2, i1* @OUT, align 1
  store float 0x40594CCCC0000000, float* @TMP2, align 4
  ret i32 0
}
