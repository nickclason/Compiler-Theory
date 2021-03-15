; ModuleID = 'TEST'
source_filename = "TEST"

@X = common global i32 0
@Z = common global i32 0
@TMP = common global i1 false

declare i1 @PUTINTEGER(i32)

define i32 @proc_0(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  store i32 %0, i32* %1, align 4
  %2 = call i32 @proc_1(i32 80)
  store i32 %2, i32* @Z, align 4
  %3 = load i32, i32* @X, align 4
  %4 = icmp eq i32 %3, 100
  br i1 %4, label %5, label %6

5:                                                ; preds = %entrypoint
  ret i32 1

6:                                                ; preds = %entrypoint
  %7 = load i32, i32* @X, align 4
  %8 = icmp eq i32 %7, 0
  br i1 %8, label %9, label %10

9:                                                ; preds = %6
  ret i32 -1

10:                                               ; preds = %6
  ret i32 0
}

define i32 @proc_1(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  store i32 %0, i32* %1, align 4
  store i32 100, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = call i1 @PUTINTEGER(i32 %3)
  store i1 %4, i1* @TMP, align 1
  ret i32 0
}

define i32 @main() {
entrypoint:
  store i32 0, i32* @X, align 4
  %0 = call i32 @proc_0(i32 81)
  store i32 %0, i32* @Z, align 4
  %1 = load i32, i32* @X, align 4
  %2 = call i1 @PUTINTEGER(i32 %1)
  store i1 %2, i1* @TMP, align 1
  ret i32 0
}
