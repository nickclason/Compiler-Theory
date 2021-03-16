; ModuleID = 'LOOPFIB'
source_filename = "LOOPFIB"

@X = common global i32 0
@I = common global i32 0
@MAX = common global i32 0
@TMP = common global i32 0
@OUT = common global i1 false

declare i1 @PUTINTEGER(i32)

define i32 @proc_0(i32 %0) {
entrypoint:
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
}
