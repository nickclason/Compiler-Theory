; ModuleID = 'TEST'
source_filename = "TEST"

@ARR = common global [0 x i32] zeroinitializer
@I = common global i32 0
@X = common global i32 0
@TMP = common global i1 false

@formatString = private constant [2 x i8] c"%d" 
declare i32 @printf(i8*, i32)
declare i1 @PUTINTEGER(i32)

define i32 @main() {
entrypoint:
  store i32 0, i32* @I, align 4
  br label %0

0:                                                ; preds = %10, %entrypoint
  %1 = load i32, i32* @I, align 4
  %2 = icmp slt i32 %1, 10
  br i1 %2, label %3, label %8

3:                                                ; preds = %0
  %4 = load i32, i32* @I, align 4
  %5 = icmp sge i32 %4, 0
  %6 = icmp slt i32 %4, 0
  %7 = and i1 %5, %6
  br i1 %7, label %10, label %9

8:                                                ; preds = %0
  store i32 0, i32* @I, align 4
  br label %15

9:                                                ; preds = %3
  br label %10

10:                                               ; preds = %9, %3
  %11 = getelementptr inbounds [0 x i32], [0 x i32]* @ARR, i32 0, i32 %4
  %12 = load i32, i32* @I, align 4
  store i32 %12, i32* %11, align 4
  %13 = load i32, i32* @I, align 4
  %14 = add i32 %13, 1
  store i32 %14, i32* @I, align 4
  br label %0

15:                                               ; preds = %25, %8
  %16 = load i32, i32* @I, align 4
  %17 = icmp slt i32 %16, 10
  br i1 %17, label %18, label %23

18:                                               ; preds = %15
  %19 = load i32, i32* @I, align 4
  %20 = icmp sge i32 %19, 0
  %21 = icmp slt i32 %19, 0
  %22 = and i1 %20, %21
  br i1 %22, label %25, label %24

23:                                               ; preds = %15
  ret i32 0

24:                                               ; preds = %18
  br label %25

25:                                               ; preds = %24, %18
  %26 = getelementptr inbounds [0 x i32], [0 x i32]* @ARR, i32 0, i32 %19
  %27 = load i32, i32* %26, align 4
  %call = call i32 (i8*, i32) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @formatString , i32 0, i32 0), i32 %27)
  %28 = load i32, i32* @I, align 4
  %29 = add i32 %28, 1
  store i32 %29, i32* @I, align 4
  br label %15
}
