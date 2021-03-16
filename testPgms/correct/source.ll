; ModuleID = 'MYTESTPROGRAM'
source_filename = "MYTESTPROGRAM"

@I = common global i32 0
@C = common global i8* null
@0 = private unnamed_addr constant [2 x i8] c"A\00", align 1

declare i1 @PUTINTEGER(i32)

define i32 @main() {
entrypoint:
  store i32 100, i32* @I, align 4
  store i8* getelementptr inbounds ([2 x i8], [2 x i8]* @0, i32 0, i32 0), i8** @C, align 8
  %0 = load i32, i32* @I, align 4
  %1 = icmp sgt i32 %0, 100
  br i1 %1, label %2, label %3

2:                                                ; preds = %entrypoint
  store i32 1110, i32* @I, align 4
  br label %3

3:                                                ; preds = %2, %entrypoint
  ret i32 0
}
